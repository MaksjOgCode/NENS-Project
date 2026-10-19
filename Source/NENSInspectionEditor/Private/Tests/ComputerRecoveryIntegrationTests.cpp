#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/StrongObjectPtr.h"
#include "ComputerRecoveryComponent.h"
#include "ComputerRecoveryWidget.h"
#include "ClockInspectionComponent.h"
#include "InspectionSceneData.h"
#include "InspectionBlueprintLibrary.h"
#include "InteractionSessionSubsystem.h"
#include "MinigameComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest
{
bool Invoke(UObject*,FName,void* = nullptr);
bool Click(UUserWidget*,FName,FVector2D = FVector2D(.5,.5));
USceneComponent* SceneComponent(AActor*,FName);
void Escape();
}
namespace RecoveryIntegration
{
using namespace WiresTest;
#define REQUIRE_RECOVERY(C,M) if (!Test->TestTrue(M,!!(C))) return true
void Layout(FAutomationTestBase* Test,UComputerRecoveryWidget* UI)
{
    const auto& View=UI->GetCachedGeometry();
    for (FName Name : {TEXT("Canvas_Minigame"),TEXT("TxtTitle"),TEXT("Txt_State"),TEXT("Txt_ProgramHint"),TEXT("Btn_LoadCards"),TEXT("Btn_Restore"),TEXT("BtnClose"),TEXT("Text_RecoveryControls")})
    {
        auto* Part=UI->WidgetTree->FindWidget(Name); if (!Test->TestNotNull(Name.ToString(),Part)) continue;
        const auto& G=Part->GetCachedGeometry();
        const auto A=View.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));
        const auto B=View.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        Test->TestTrue(TEXT("Fits viewport: ")+Name.ToString(),G.GetLocalSize().X>0 && G.GetLocalSize().Y>0 && A.X>=-1 && A.Y>=-1 && B.X<=View.GetLocalSize().X+1 && B.Y<=View.GetLocalSize().Y+1);
        if (Name==TEXT("Canvas_Minigame")) Test->TestTrue(TEXT("Terminal centered"),((A+B)*.5).Equals(View.GetLocalSize()*.5,1));
        if (auto* Copy=Cast<UTextBlock>(Part)) Test->TestTrue(TEXT("Shared font"),Copy->GetFont().FontObject && Copy->GetFont().FontObject->GetFName()==TEXT("F_IMFellEnglish"));
    }
    for (FName Name : {TEXT("MinigameGradient"),TEXT("MinigameMist")})
    {
        auto* Effect=UI->WidgetTree->FindWidget(Name);
        if (Test->TestNotNull(TEXT("Procedural effect"),Effect)) Test->TestEqual(TEXT("Effect cannot intercept clicks"),Effect->GetVisibility(),ESlateVisibility::HitTestInvisible);
    }
}
class FFlow : public IAutomationLatentCommand
{
public:
    explicit FFlow(FAutomationTestBase* T):Test(T),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>65) { Previous.Reset(); Test->AddError(FString::Printf(TEXT("Recovery flow timed out at step %d"),Step)); return true; }
        if (Now<Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World=GEditor->PlayWorld; auto* PC=World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer() || !PC->GetPawn()) return false;
        auto* P=UComputerRecoverySubsystem::From(PC);
        auto* Shared=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        switch (Step)
        {
        case 0:
        {
            Test->TestEqual(TEXT("New PIE bag starts empty"),P->GetInventoryCount(TEXT("PunchCard")),0);
            Test->TestFalse(TEXT("New PIE systems start offline"),P->IsSystemReady(EComputerSystem::Power));
            auto* Class=LoadClass<AActor>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_DataComputer.BP_Props_DataComputer_C"));
            REQUIRE_RECOVERY(Class,TEXT("Main computer class exists"));
            Owner=World->SpawnActor<AActor>(Class,FVector(3000,3000,0),FRotator::ZeroRotator);
            REQUIRE_RECOVERY(Owner.IsValid(),TEXT("Main computer spawned"));
            Source=Owner->FindComponentByClass<UComputerRecoveryComponent>();
            REQUIRE_RECOVERY(Source.IsValid(),TEXT("Main computer has recovery component"));
            Test->TestEqual(TEXT("All existing punch cards required"),P->GetRequiredCardCount(),2);
            for (TActorIterator<AActor> It(World);It;++It)
                if (auto* C=It->FindComponentByClass<USearchableComponent>())
                {
                    if (C->Scene && C->Scene->GetFName()==TEXT("DA_Inspection_Wardrobe")) Wardrobe=C;
                    if (auto* Clock=Cast<UClockInspectionComponent>(C)) ClockSource=Clock;
                }
            REQUIRE_RECOVERY(Wardrobe.IsValid() && ClockSource.IsValid(),TEXT("Both actual card scenes exist on gameplay map"));
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction=C;
            auto* Sphere=Cast<USphereComponent>(SceneComponent(Owner.Get(),TEXT("Sphere_Interaction")));
            REQUIRE_RECOVERY(Sphere && Interaction.IsValid(),TEXT("Standard sphere and E route"));
            Near=Sphere->GetComponentLocation()+FVector(Sphere->GetScaledSphereRadius()*.7,0,0); Near.Z=PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near,false,nullptr,ETeleportType::TeleportPhysics);
            break;
        }
        case 1:
            Invoke(Interaction.Get(),TEXT("UpdateInteractionTarget")); Invoke(Interaction.Get(),TEXT("TryInteract"));
            UI=Source->GetActiveWidget(); REQUIRE_RECOVERY(UI.IsValid(),TEXT("E opens main computer from inside ring"));
            Test->TestTrue(TEXT("Smooth entry begins transparent"),UI->IsOpening() && UI->GetRenderOpacity()==0);
            Test->TestTrue(TEXT("Input owned by current room modal"),Shared->IsActive(UI.Get()) && PC->IsMoveInputIgnored() && PC->bShowMouseCursor && PC->GetPawn()->GetActorLocation().Equals(Near));
            Test->TestFalse(TEXT("Cannot open same terminal twice"),Source->OpenComputer(PC->GetPawn()));
            Test->TestFalse(TEXT("No cards or systems cannot restore"),P->BeginRestoration());
            break;
        case 2:
        {
            if (!UI->IsInteractive()) return false;
            Layout(Test,UI.Get());
            Test->TestFalse(TEXT("Restore button locked"),UI->WidgetTree->FindWidget(TEXT("Btn_Restore"))->GetIsEnabled());
            UI->ReportSuccess(); Test->TestFalse(TEXT("Terminal cannot fake a puzzle win"),UI->HasReportedSuccess());
            TArray<UUserWidget*> HUD; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,HUD,URecoveryObjectiveWidget::StaticClass(),true);
            Test->TestEqual(TEXT("One objective panel"),HUD.Num(),1);
            if (!HUD.IsEmpty()) Test->TestTrue(TEXT("Objective hidden during modal"),HUD[0]->GetRenderOpacity()<.05f);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/offline.png"),true,false);
            break;
        }
        case 3:
            REQUIRE_RECOVERY(Wardrobe->TryCollect(TEXT("Wardrobe_PunchCard")),TEXT("Real wardrobe source adds first card to bag"));
            Test->TestEqual(TEXT("Inventory updated"),P->GetInventoryCount(TEXT("PunchCard")),1);
            REQUIRE_RECOVERY(Click(UI.Get(),TEXT("Btn_LoadCards")),TEXT("Load button clicked"));
            Test->TestEqual(TEXT("Button loads first card"),P->GetLoadedCardCount(),1);
            Test->TestFalse(TEXT("Partial program cannot restore"),P->CanRestore());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/partial.png"),true,false);
            Previous.Reset(UI.Get()); Escape();
            Test->TestTrue(TEXT("Escape retains input during exit fade"),UI->IsClosing() && Shared->IsOpen() && PC->IsMoveInputIgnored());
            break;
        case 4:
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Exit releases gameplay input"),Shared->IsOpen() || PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Main computer remains interactable"),UInspectionBlueprintLibrary::IsInteractionAvailable(Owner.Get()));
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Source->GetActiveWidget();
            REQUIRE_RECOVERY(UI.IsValid(),TEXT("Terminal reopens"));
            Previous->RequestClose(); Previous->ReportSuccess(); Previous.Reset();
            Test->TestTrue(TEXT("Old widget cannot close new session"),Shared->IsActive(UI.Get()));
            Test->TestEqual(TEXT("Loaded card survives reentry"),P->GetLoadedCardCount(),1);
            break;
        case 5:
        {
            if (!UI->IsInteractive()) return false;
            auto* C=ClockSource.Get();
            Test->TestFalse(TEXT("Clock card blocked before puzzle solved"),C->TryCollect(TEXT("FloorClock_PunchCard")));
            C->PlaceHand(EClockHand::Short); C->PlaceHand(EClockHand::Long);
            C->RotateHand(EClockHand::Short,C->Puzzle->ShortHand.TargetAngle); C->RotateHand(EClockHand::Long,C->Puzzle->LongHand.TargetAngle);
            REQUIRE_RECOVERY(C->TryCollect(TEXT("FloorClock_PunchCard")),TEXT("Solved clock grants second card"));
            Click(UI.Get(),TEXT("Btn_LoadCards"));
            Test->TestEqual(TEXT("All existing cards loaded"),P->GetLoadedCardCount(),2);
            P->MarkSystemReady(EComputerSystem::Power,TEXT("test-wires"));
            P->MarkSystemReady(EComputerSystem::Fuel,TEXT("test-valves"));
            Test->TestFalse(TEXT("Missing display blocks recovery despite full program"),P->CanRestore());
            P->MarkSystemReady(EComputerSystem::Display,TEXT("test-disks"));
            Test->TestTrue(TEXT("Complete systems and cards unlock restore button"),P->CanRestore() && UI->WidgetTree->FindWidget(TEXT("Btn_Restore"))->GetIsEnabled());
            if (auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget())) { OriginalSize=Window->GetSizeInScreen(); Window->Resize(FVector2D(1000,900)); }
            break;
        }
        case 6:
            Layout(Test,UI.Get());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/ready-compact.png"),true,false);
            break;
        case 7:
            if (auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget())) Window->Resize(OriginalSize);
            break;
        case 8:
            Click(UI.Get(),TEXT("Btn_Restore"));
            REQUIRE_RECOVERY(P->IsRestoring(),TEXT("Real button starts recovery"));
            Test->TestFalse(TEXT("Repeated start ignored"),P->BeginRestoration());
            Next=Now+.6; ++Step; return false;
        case 9:
            Test->TestTrue(TEXT("Program has intermediate progress"),P->GetRestoreProgress()>0 && P->GetRestoreProgress()<1);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/booting.png"),true,false);
            Click(UI.Get(),TEXT("BtnClose")); break;
        case 10:
            if (Source->GetActiveWidget()) return false;
            Test->TestTrue(TEXT("Program runs independently of closed widget"),P->IsRestoring());
            Test->TestFalse(TEXT("Gameplay available during recovery"),Shared->IsOpen() || PC->IsMoveInputIgnored());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/objective.png"),true,false);
            Next=Now+3; ++Step; return false;
        case 11:
            Test->TestTrue(TEXT("Recovery completes while terminal is closed"),P->IsRestored() && !P->IsRestoring());
            Test->TestEqual(TEXT("Completed progress stays at 100 percent"),P->GetRestoreProgress(),1.f);
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Source->GetActiveWidget();
            REQUIRE_RECOVERY(UI.IsValid(),TEXT("Online terminal can reopen"));
            break;
        case 12:
            if (!UI->IsInteractive()) return false;
            Test->TestEqual(TEXT("Online state visible"),CastChecked<UTextBlock>(UI->WidgetTree->FindWidget(TEXT("Txt_State")))->GetText().ToString(),FString(TEXT("COMPUTER ONLINE")));
            Test->TestFalse(TEXT("Cannot run completed recovery again"),P->BeginRestoration());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ComputerRecovery/online.png"),true,false);
            break;
        case 13:
            Owner->Destroy();
            Test->TestFalse(TEXT("Owner destruction cleans modal immediately"),Shared->IsOpen() || PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Destroyed computer does not lose global goal"),P->IsRestored());
            return true;
        }
        ++Step; Next=Now+.4; return false;
    }
private:
    FAutomationTestBase* Test; double Started,Next=0; int32 Step=0;
    FVector Near; FVector2D OriginalSize;
    TWeakObjectPtr<AActor> Owner;
    TWeakObjectPtr<UComputerRecoveryComponent> Source;
    TWeakObjectPtr<UComputerRecoveryWidget> UI;
    TWeakObjectPtr<USearchableComponent> Wardrobe;
    TWeakObjectPtr<UClockInspectionComponent> ClockSource;
    TWeakObjectPtr<UActorComponent> Interaction;
    TStrongObjectPtr<UComputerRecoveryWidget> Previous;
};
#undef REQUIRE_RECOVERY
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryPIETest,"NENS.Recovery.PIEComputerFlow",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRecoveryPIETest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<RecoveryIntegration::FFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
