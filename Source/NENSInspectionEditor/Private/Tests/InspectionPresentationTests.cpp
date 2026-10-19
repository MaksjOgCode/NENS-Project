#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "SearchableComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
void PressEscape()
{
    auto& Slate = FSlateApplication::Get();
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
    Slate.ProcessKeyUpEvent(FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
}
class FInspectionPresentationFlow : public IAutomationLatentCommand
{
public:
    explicit FInspectionPresentationFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 40) { Test->AddError(TEXT("Inspection presentation timed out")); return true; }
        if (Now < Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer()) return false;
        auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (Step == 0)
        {
            for (TActorIterator<AActor> It(World); It; ++It)
                if (auto* Component = It->FindComponentByClass<USearchableComponent>())
                    if (Component->Scene && Component->Scene->GetFName() == TEXT("DA_Inspection_Wardrobe"))
                    { Owner = World->SpawnActor<AActor>(It->GetClass(), FVector(5000,5000,0), FRotator::ZeroRotator); break; }
            if (!Test->TestNotNull(TEXT("Independent presentation fixture"), Owner.Get())) return true;
            Source = Owner->FindComponentByClass<USearchableComponent>();
            if (!Test->TestTrue(TEXT("Inspection opens"), Session->Open(Source.Get()))) return true;
            auto* UI = Session->GetActiveWidget();
            Test->TestTrue(TEXT("Inspection starts transparent and opening"), UI->IsOpening() && UI->GetRenderOpacity() == 0);
            Test->TestFalse(TEXT("Invisible pickup cannot be collected during entry"), UI->CollectPickup(Source->Scene->Pickups[0].PickupID));
            Step++; Next = Now + .08; return false;
        }
        switch (Step)
        {
        case 1:
        {
            auto* UI = Session->GetActiveWidget();
            if (!Test->TestNotNull(TEXT("Opening widget still owns session"), UI)) return true;
            Test->TestTrue(TEXT("Opening has an actual intermediate opacity"), UI->GetRenderOpacity() > 0 && UI->GetRenderOpacity() < 1);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("InspectionTransitions/scene-opening.png"), true, false);
            Step++; Next = Now + .04; return false;
        }
        case 2:
        {
            auto* UI = Session->GetActiveWidget(); OldWidget.Reset(UI);
            const float Opacity = UI->GetRenderOpacity();
            PressEscape();
            Test->TestTrue(TEXT("Escape reverses opening from its current opacity"), UI->IsClosing() && FMath::IsNearlyEqual(Opacity, UI->GetRenderOpacity()));
            Session->Close(); PressEscape();
            Test->TestTrue(TEXT("Repeated close requests do not restart or flash the fade"), FMath::IsNearlyEqual(Opacity, UI->GetRenderOpacity()));
            Test->TestTrue(TEXT("Entry cancellation retains cursor and movement lock while fading"), PC->bShowMouseCursor && PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Another inspection cannot overlap exit"), Session->Open(Source.Get()));
            Step++; Next = Now + .4; return false;
        }
        case 3:
            Test->TestFalse(TEXT("Cancelled opening closes and restores input"), Session->IsOpen() || PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Cancelling entry does not collect anything"), Source->IsComplete());
            if (!Test->TestTrue(TEXT("Inspection reopens normally"), Session->Open(Source.Get()))) return true;
            Session->FinishClose(OldWidget.Get());
            Test->TestTrue(TEXT("Old completion cannot close a new inspection"), Session->IsOpen());
            Step++; Next = Now + .5; return false;
        case 4:
        {
            auto* UI = Session->GetActiveWidget();
            Test->TestTrue(TEXT("Entry finishes fully opaque and interactive"), UI && UI->IsInspectionInteractive() && UI->GetRenderOpacity() == 1);
            Session->Close();
            Test->TestTrue(TEXT("Normal close begins a visible fade"), UI->IsClosing() && UI->GetRenderOpacity() == 1);
            Test->TestFalse(TEXT("Closing scene rejects collection"), UI->CollectPickup(Source->Scene->Pickups[0].PickupID));
            Step++; Next = Now + .07; return false;
        }
        case 5:
        {
            auto* UI = Session->GetActiveWidget();
            if (!Test->TestNotNull(TEXT("Exit retains widget until its fade completes"), UI)) return true;
            Test->TestTrue(TEXT("Exit has an actual intermediate opacity"), UI->GetRenderOpacity() > 0 && UI->GetRenderOpacity() < 1);
            Test->TestTrue(TEXT("Exit keeps input locked"), PC->IsMoveInputIgnored() && PC->bShowMouseCursor);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("InspectionTransitions/scene-closing.png"), true, false);
            PressEscape();
            Step++; Next = Now + .4; return false;
        }
        case 6:
            Test->TestFalse(TEXT("Normal fade completes and restores input"), Session->IsOpen() || PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            if (!Test->TestTrue(TEXT("Inspection opens for external-removal check"), Session->Open(Source.Get()))) return true;
            Session->GetActiveWidget()->RemoveFromParent();
            Test->TestFalse(TEXT("External removal tears down immediately"), Session->IsOpen() || PC->IsMoveInputIgnored());
            if (!Test->TestTrue(TEXT("Inspection opens for owner-end check"), Session->Open(Source.Get()))) return true;
            Owner->Destroy();
            Test->TestFalse(TEXT("Owner destruction cancels presentation and restores input"), Session->IsOpen() || PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    TWeakObjectPtr<AActor> Owner;
    TWeakObjectPtr<USearchableComponent> Source;
    TStrongObjectPtr<UInspectionSceneWidget> OldWidget;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectionPresentationTest, "NENS.Inspection.Presentation.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInspectionPresentationTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FInspectionPresentationFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
