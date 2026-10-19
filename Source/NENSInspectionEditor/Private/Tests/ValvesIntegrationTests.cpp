#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/TextBlock.h"
#include "Components/SphereComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "MinigameComponent.h"
#include "ValvesMinigameWidget.h"
#include "InteractionSessionSubsystem.h"
#include "InspectionBlueprintLibrary.h"
#include "UObject/StrongObjectPtr.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest
{
bool Invoke(UObject*,FName,void* = nullptr);
bool Click(UUserWidget*,FName,FVector2D = FVector2D(.5,.5));
USceneComponent* SceneComponent(AActor*,FName);
void Escape();
}
namespace ValvesTest
{
#define CHECK_TEST(C,M) if (!Test->TestTrue(M,!!(C))) return true
using namespace WiresTest;
UValveControlWidget* Wheel(UMinigameWidget* UI,FName Name)
{ return UI ? Cast<UValveControlWidget>(UI->WidgetTree->FindWidget(Name)) : nullptr; }
void Turn(UMinigameWidget* UI,FName Name) { Click(Wheel(UI,Name),TEXT("Button_0")); }
void CheckLayout(FAutomationTestBase* Test,UMinigameWidget* UI)
{
    const auto& View=UI->GetCachedGeometry();
    for (FName Name : {TEXT("Canvas_Minigame"),TEXT("Board"),TEXT("TxtTitle"),TEXT("TxtStatus"),TEXT("Text_ValvesControls"),TEXT("BtnClose"),TEXT("Valve_Left"),TEXT("Valve_Top"),TEXT("Valve_Right")})
    {
        auto* Part=UI->WidgetTree->FindWidget(Name);
        if (!Test->TestNotNull(Name.ToString(),Part)) continue;
        const auto& G=Part->GetCachedGeometry();
        const auto A=View.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector)), B=View.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        Test->TestTrue(TEXT("Fits viewport: ")+Name.ToString(),G.GetLocalSize().X>0 && G.GetLocalSize().Y>0 && A.X>=-1 && A.Y>=-1 && B.X<=View.GetLocalSize().X+1 && B.Y<=View.GetLocalSize().Y+1);
        if (Name==TEXT("Canvas_Minigame")) Test->TestTrue(TEXT("Valve scene centered"),((A+B)*.5).Equals(View.GetLocalSize()*.5,1));
        if (auto* T=Cast<UTextBlock>(Part)) Test->TestTrue(TEXT("Shared inspection font"),T->GetFont().FontObject && T->GetFont().FontObject->GetFName()==TEXT("F_IMFellEnglish"));
        if (auto* W=Cast<UValveControlWidget>(Part)) Test->TestNotNull(TEXT("Visible orientation overlay"),W->WidgetTree->FindWidget(TEXT("ValveOrientation")));
    }
    for (FName Name : {TEXT("MinigameGradient"),TEXT("MinigameMist")})
    {
        auto* Effect=UI->WidgetTree->FindWidget(Name);
        Test->TestTrue(TEXT("Procedural effect passes clicks"),Effect && Effect->GetVisibility()==ESlateVisibility::HitTestInvisible);
    }
}
class FFlow : public IAutomationLatentCommand
{
public:
    FFlow(FAutomationTestBase* T,bool Standalone):Test(T),bStandalone(Standalone),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>60) { Test->AddError(FString::Printf(TEXT("Valve flow timed out at %d"),Step)); return true; }
        if (Now<Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World=GEditor->PlayWorld; auto* PC=World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer()) return false;
        auto* Shared=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        if (Step==0)
        {
            if (bStandalone)
            {
                TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UValvesMinigameWidget::StaticClass(),true);
                if (Widgets.IsEmpty()) return false;
                UI=Cast<UMinigameWidget>(Widgets[0]);
                Test->TestFalse(TEXT("Original valve level remains independent"),Shared->IsOpen());
                Step=6; Next=Now+.5; return false;
            }
            auto* Class=LoadClass<AActor>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_PickMeBox.BP_Props_PickMeBox_C"));
            CHECK_TEST(Class,TEXT("Box class"));
            Owner=World->SpawnActor<AActor>(Class,FVector(3000,3000,0),FRotator::ZeroRotator);
            CHECK_TEST(Owner.IsValid(),TEXT("Box spawned"));
            Source=Owner->FindComponentByClass<UMinigameComponent>();
            CHECK_TEST(Source.IsValid(),TEXT("Box has minigame component"));
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction=C;
            auto* Sphere=Cast<USphereComponent>(SceneComponent(Owner.Get(),TEXT("Sphere_Interaction")));
            CHECK_TEST(Sphere && Interaction.IsValid(),TEXT("Inherited E interaction and sphere"));
            auto Near=Sphere->GetComponentLocation()+FVector(Sphere->GetScaledSphereRadius()*.7,0,0);
            Near.Z=PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near,false,nullptr,ETeleportType::TeleportPhysics);
        }
        else if (Step==1)
        {
            Invoke(Interaction.Get(),TEXT("UpdateInteractionTarget")); Invoke(Interaction.Get(),TEXT("TryInteract"));
            UI=Source->GetActiveWidget(); CHECK_TEST(UI.IsValid(),TEXT("E inside the ring opens valves"));
            Test->TestTrue(TEXT("Entry starts transparent"),UI->IsOpening() && UI->GetRenderOpacity()==0);
            Test->TestTrue(TEXT("Same world and locked gameplay"),Owner->GetWorld()==World && PC->IsMoveInputIgnored() && PC->bShowMouseCursor);
            Turn(UI.Get(),TEXT("Valve_Left"));
            Test->TestEqual(TEXT("Entry blocks premature clicks"),Wheel(UI.Get(),TEXT("Valve_Left"))->ReadAngle(TEXT("TargetAngle")),0.);
            Step++; Next=Now+.12; return false;
        }
        else if (Step==2)
        {
            Test->TestTrue(TEXT("Entry has intermediate opacity"),UI->GetRenderOpacity()>0 && UI->GetRenderOpacity()<1);
            const float Opacity=UI->GetRenderOpacity(); UI->RequestClose(); UI->RequestClose();
            Test->TestTrue(TEXT("Escape during entry reverses from current opacity"),UI->IsClosing() && UI->GetRenderOpacity()==Opacity && Shared->IsActive(UI.Get()));
            Test->TestTrue(TEXT("Exit still locks movement"),PC->IsMoveInputIgnored());
            Previous.Reset(UI.Get());
        }
        else if (Step==3)
        {
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Fade eventually restores controls"),PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Abort leaves box incomplete"),Source->IsComplete());
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Source->GetActiveWidget();
            CHECK_TEST(UI.IsValid(),TEXT("Aborted box reopens"));
            Previous->RequestClose(); Previous->ReportSuccess();
            Test->TestTrue(TEXT("Stale callbacks do not close new modal"),Shared->IsActive(UI.Get()));
        }
        else if (Step==4)
        {
            if (UI->IsOpening()) return false;
            CheckLayout(Test,UI.Get());
            Test->TestEqual(TEXT("Fresh attempt resets angles"),Wheel(UI.Get(),TEXT("Valve_Left"))->ReadAngle(TEXT("TargetAngle")),0.);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ValvesIntegration/valves-wide.png"),true,false);
        }
        else if (Step==5)
        {
            auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget());
            if (PreviewStep==0)
            {
                if (Window) { OriginalSize=Window->GetSizeInScreen(); Window->Resize(FVector2D(1000,900)); }
                PreviewStep++; Next=Now+.4; return false;
            }
            if (PreviewStep==1)
            {
                CheckLayout(Test,UI.Get());
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ValvesIntegration/valves-compact.png"),true,false);
                PreviewStep++; Next=Now+.4; return false;
            }
            CheckLayout(Test,UI.Get());
            if (Window) Window->Resize(OriginalSize);
        }
        else if (Step==6) { Turn(UI.Get(),TEXT("Valve_Left")); }
        else if (Step==7)
        {
            Test->TestTrue(TEXT("Left pointer aligned after one turn"),Wheel(UI.Get(),TEXT("Valve_Left"))->IsAligned());
            Turn(UI.Get(),TEXT("Valve_Top")); Turn(UI.Get(),TEXT("Valve_Right"));
        }
        else if (Step==8)
        {
            Test->TestFalse(TEXT("One top turn is wrong"),Wheel(UI.Get(),TEXT("Valve_Top"))->IsAligned());
            Test->TestFalse(TEXT("Wrong arrangement cannot win"),UI->HasReportedSuccess());
            Turn(UI.Get(),TEXT("Valve_Top")); Turn(UI.Get(),TEXT("Valve_Right"));
        }
        else if (Step==9)
        {
            Test->TestTrue(TEXT("Top needs two turns"),Wheel(UI.Get(),TEXT("Valve_Top"))->IsAligned());
            Test->TestFalse(TEXT("Right needs three turns"),Wheel(UI.Get(),TEXT("Valve_Right"))->IsAligned());
            Turn(UI.Get(),TEXT("Valve_Right"));
            Test->TestFalse(TEXT("Rotating valve cannot complete puzzle"),UI->HasReportedSuccess());
        }
        else if (Step==10)
        {
            CHECK_TEST(UI->HasReportedSuccess(),TEXT("Three settled orientations report success"));
            Test->TestTrue(TEXT("Success remains visible before fade"),UI->GetRenderOpacity()>.99f);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(bStandalone ? TEXT("ValvesIntegration/valves-standalone-solved.png") : TEXT("ValvesIntegration/valves-solved.png")),true,false);
            Step++; Next=Now+1.3; return false;
        }
        else if (Step==11)
        {
            Test->TestFalse(TEXT("Success closes smoothly"),UI->IsInViewport());
            Test->TestFalse(TEXT("Success restores controls"),PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            if (bStandalone) return true;
            Test->TestTrue(TEXT("Box result stored"),Source->IsComplete());
            Test->TestFalse(TEXT("Completed box cannot reopen"),Source->OpenMinigame(PC->GetPawn()));
            for (FName Name : {TEXT("Ring_Visual"),TEXT("Widget_InteractionPrompt")})
                Test->TestTrue(TEXT("Completed indicators retired"),SceneComponent(Owner.Get(),Name)->bHiddenInGame);
            Test->TestFalse(TEXT("Box artwork stays visible"),Owner->IsHidden());
            OtherOwner=World->SpawnActor<AActor>(Owner->GetClass(),FVector(5000,5000,0),FRotator::ZeroRotator);
            auto* Other=OtherOwner->FindComponentByClass<UMinigameComponent>();
            CHECK_TEST(Other->OpenMinigame(PC->GetPawn()),TEXT("Another box independent"));
            Other->GetActiveWidget()->RemoveFromParent();
            Test->TestFalse(TEXT("External widget removal restores session"),Shared->IsOpen());
            CHECK_TEST(Other->OpenMinigame(PC->GetPawn()),TEXT("Externally removed attempt can reopen"));
            OtherOwner->Destroy();
            Test->TestFalse(TEXT("Owner destruction closes immediately even during entry"),Shared->IsOpen() || PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            return true;
        }
        Step++; Next=Now+.5; return false;
    }
private:
    FAutomationTestBase* Test;
    bool bStandalone;
    double Started,Next=0;
    int32 Step=0;
    int32 PreviewStep=0;
    FVector2D OriginalSize;
    TWeakObjectPtr<AActor> Owner,OtherOwner;
    TWeakObjectPtr<UMinigameComponent> Source;
    TWeakObjectPtr<UActorComponent> Interaction;
    TWeakObjectPtr<UMinigameWidget> UI;
    TStrongObjectPtr<UMinigameWidget> Previous;
};
#undef CHECK_TEST
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoxValvesTest,"NENS.Interaction.Minigame.BoxValves",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBoxValvesTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ValvesTest::FFlow>(this,false));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStandaloneValvesTest,"NENS.Interaction.Minigame.OriginalValvesLevel",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStandaloneValvesTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/MiniGameValves2/L_valves1"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ValvesTest::FFlow>(this,true));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
