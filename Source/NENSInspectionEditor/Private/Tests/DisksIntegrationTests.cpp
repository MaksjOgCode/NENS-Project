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
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "MinigameComponent.h"
#include "DisksMinigameWidget.h"
#include "InteractionSessionSubsystem.h"
#include "InspectionBlueprintLibrary.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest
{
bool Invoke(UObject*,FName,void* = nullptr);
bool Click(UUserWidget*,FName,FVector2D = FVector2D(.5,.5));
USceneComponent* SceneComponent(AActor*,FName);
void Escape();
int32 ReadInt(UObject*,FName);
}
namespace DisksTest
{
using namespace WiresTest;
#define REQUIRE_DISKS(C,M) if (!Test->TestTrue(M,!!(C))) return true
float Angle(UDisksMinigameWidget* UI,int32 Ring)
{
    const FName Names[]={TEXT("Ring_Inner"),TEXT("Ring_Middle"),TEXT("Ring_Outer")};
    auto* Image=UI ? Cast<UImage>(UI->WidgetTree->FindWidget(Names[Ring])) : nullptr;
    return Image ? Image->GetRenderTransformAngle() : -999;
}
bool Press(UDisksMinigameWidget* UI,FVector2D Fraction,FVector2D& Point)
{
    auto* Board=UI->WidgetTree->FindWidget(TEXT("Board"));
    auto& Slate=FSlateApplication::Get(); auto Window=Slate.FindWidgetWindow(UI->TakeWidget());
    if (!Board || !Window) return false;
    Point=Board->GetCachedGeometry().LocalToAbsolute(Board->GetCachedGeometry().GetLocalSize()*Fraction);
    Slate.ProcessMouseMoveEvent(FPointerEvent(0,Point,Point,TSet<FKey>(),EKeys::Invalid,0,FModifierKeysState()),false);
    TSet<FKey> Buttons; Buttons.Add(EKeys::LeftMouseButton);
    Slate.ProcessMouseButtonDownEvent(Window->GetNativeWindow(),FPointerEvent(0,Point,Point,Buttons,EKeys::LeftMouseButton,0,FModifierKeysState()));
    return true;
}
void Release(FVector2D Point)
{
    FSlateApplication::Get().ProcessMouseButtonUpEvent(FPointerEvent(0,Point,Point,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState()));
}
void Layout(FAutomationTestBase* Test,UDisksMinigameWidget* UI)
{
    const auto& View=UI->GetCachedGeometry();
    for (FName Name : {TEXT("Canvas_Minigame"),TEXT("ScenePages"),TEXT("TxtTitle"),TEXT("Text_DisksControls"),TEXT("BtnClose")})
    {
        auto* Part=UI->WidgetTree->FindWidget(Name);
        if (!Test->TestNotNull(Name.ToString(),Part)) continue;
        const auto& G=Part->GetCachedGeometry(); const auto A=View.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));
        const auto B=View.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        Test->TestTrue(TEXT("Fits viewport: ")+Name.ToString(),A.X>=-1 && A.Y>=-1 && B.X<=View.GetLocalSize().X+1 && B.Y<=View.GetLocalSize().Y+1);
        if (Name==TEXT("Canvas_Minigame")) Test->TestTrue(TEXT("Whole presentation centered"),((A+B)*.5).Equals(View.GetLocalSize()*.5,1));
        if (auto* Text=Cast<UTextBlock>(Part)) Test->TestTrue(TEXT("Shared font"),Text->GetFont().FontObject && Text->GetFont().FontObject->GetFName()==TEXT("F_IMFellEnglish"));
    }
    for (FName Name : {TEXT("MinigameGradient"),TEXT("MinigameMist")})
        Test->TestNotNull(TEXT("Common procedural effect"),UI->WidgetTree->FindWidget(Name));
    if (UI->IsShowingInspection())
        for (FName Name : {TEXT("Image_Container"),TEXT("Btn_Cabinet")})
        {
            const auto Size=UI->WidgetTree->FindWidget(Name)->GetCachedGeometry().GetLocalSize();
            Test->TestTrue(TEXT("Visible finite inspection geometry"),FMath::IsFinite(Size.X) && FMath::IsFinite(Size.Y) && Size.X>0 && Size.Y>0);
        }
}
class FFlow : public IAutomationLatentCommand
{
public:
    FFlow(FAutomationTestBase* T,bool Standalone):Test(T),bStandalone(Standalone),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        double Now=FPlatformTime::Seconds();
        if (Now-Started>75) { Test->AddError(FString::Printf(TEXT("Disk test timed out at step %d ring %d"),Step,Ring)); return true; }
        if (Now<Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World=GEditor->PlayWorld; auto* PC=World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer()) return false;
        auto* Shared=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        if (Step==0)
        {
            if (bStandalone)
            {
                TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UDisksMinigameWidget::StaticClass(),true);
                if (Widgets.IsEmpty()) return false;
                UI=Cast<UDisksMinigameWidget>(Widgets[0]);
                Test->TestFalse(TEXT("Developer level opens directly onto disks"),UI->IsShowingInspection());
                Test->TestFalse(TEXT("Developer level does not need managed session"),Shared->IsOpen());
                Step=12; Next=Now+.5; return false;
            }
            auto* Class=LoadClass<AActor>(nullptr,TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_BedsideCabinet.BP_BedsideCabinet_C"));
            REQUIRE_DISKS(Class,TEXT("Cabinet class exists"));
            Owner=World->SpawnActor<AActor>(Class,FVector(3000,3000,0),FRotator::ZeroRotator);
            REQUIRE_DISKS(Owner.IsValid(),TEXT("Cabinet spawned")); Source=Owner->FindComponentByClass<UMinigameComponent>();
            REQUIRE_DISKS(Source.IsValid(),TEXT("Cabinet has minigame component"));
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction=C;
            auto* Sphere=Cast<USphereComponent>(SceneComponent(Owner.Get(),TEXT("Sphere_Interaction")));
            REQUIRE_DISKS(Sphere && Interaction.IsValid(),TEXT("Existing E/sphere route"));
            Near=Sphere->GetComponentLocation()+FVector(Sphere->GetScaledSphereRadius()*.7,0,0); Near.Z=PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near,false,nullptr,ETeleportType::TeleportPhysics);
        }
        else if (Step==1)
        {
            Invoke(Interaction.Get(),TEXT("UpdateInteractionTarget")); Invoke(Interaction.Get(),TEXT("TryInteract"));
            UI=Cast<UDisksMinigameWidget>(Source->GetActiveWidget()); REQUIRE_DISKS(UI.IsValid(),TEXT("E opens cabinet inspection"));
            Test->TestTrue(TEXT("Entry starts transparent on inspection page"),UI->IsOpening() && UI->GetRenderOpacity()==0 && UI->IsShowingInspection());
            Test->TestTrue(TEXT("Player locked in current room"),Owner->GetWorld()==World && PC->IsMoveInputIgnored() && PC->GetPawn()->GetActorLocation().Equals(Near));
            Click(UI.Get(),TEXT("Btn_Cabinet")); Test->TestFalse(TEXT("Opening blocks hotspot clicks"),UI->IsChangingStage());
        }
        else if (Step==2)
        {
            if (UI->IsOpening()) return false;
            Layout(Test,UI.Get());
            Click(UI.Get(),TEXT("Image_Container"),FVector2D(.3,.5));
            Test->TestTrue(TEXT("Bed click does not open disks"),UI->IsShowingInspection() && !UI->IsChangingStage());
            UI->ReportSuccess(); Test->TestFalse(TEXT("Inspection cannot claim puzzle success"),Source->IsComplete());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DisksIntegration/container-inspection.png"),true,false);
        }
        else if (Step==3)
        {
            REQUIRE_DISKS(Click(UI.Get(),TEXT("Btn_Cabinet")),TEXT("Right cabinet hotspot clickable"));
            Test->TestTrue(TEXT("Hotspot starts page fade"),UI->IsChangingStage() && Shared->IsActive(UI.Get()));
            Step++; Next=Now+.1; return false;
        }
        else if (Step==4)
        {
            const auto Opacity=UI->WidgetTree->FindWidget(TEXT("ScenePages"))->GetRenderOpacity();
            Test->TestTrue(TEXT("Page has intermediate fade opacity"),Opacity>0 && Opacity<1);
            Escape(); Test->TestTrue(TEXT("Escape can close mid page transition"),UI->IsClosing());
            Previous.Reset(UI.Get());
        }
        else if (Step==5)
        {
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Mid transition exit restores controls"),PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Cast<UDisksMinigameWidget>(Source->GetActiveWidget());
            REQUIRE_DISKS(UI.IsValid(),TEXT("Cancelled inspection reopens"));
            Previous->RequestClose(); Previous->ReportSuccess();
            Test->TestTrue(TEXT("Stale callbacks cannot change newer attempt"),Shared->IsActive(UI.Get()) && !Source->IsComplete());
            Previous.Reset();
        }
        else if (Step==6)
        {
            if (!UI->IsInteractive()) return false;
            Click(UI.Get(),TEXT("Btn_Cabinet"));
        }
        else if (Step==7)
        {
            if (UI->IsChangingStage()) return false;
            Test->TestFalse(TEXT("Cabinet click reaches disk page"),UI->IsShowingInspection());
            Test->TestTrue(TEXT("Stage transition retains session and position"),Shared->IsActive(UI.Get()) && PC->IsMoveInputIgnored() && PC->GetPawn()->GetActorLocation().Equals(Near));
            REQUIRE_DISKS(Press(UI.Get(),FVector2D(.5,.42),Point),TEXT("Inner disk press"));
            Test->TestEqual(TEXT("Pointer selects inner ring"),ReadInt(UI.Get(),TEXT("ActiveRing")),0);
        }
        else if (Step==8)
        {
            Test->TestTrue(TEXT("Held click rotates original Blueprint ring"),Angle(UI.Get(),0)>120);
            Escape(); Test->TestTrue(TEXT("Escape while held starts exit"),UI->IsClosing());
        }
        else if (Step==9)
        {
            if (Source->GetActiveWidget()) return false;
            Release(Point);
            Test->TestFalse(TEXT("Held exit restores controls"),PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Cast<UDisksMinigameWidget>(Source->GetActiveWidget());
            REQUIRE_DISKS(UI.IsValid(),TEXT("Aborted puzzle reopens"));
            Test->TestTrue(TEXT("Reentry begins at room inspection"),UI->IsShowingInspection());
            Test->TestEqual(TEXT("Unfinished disks reset"),Angle(UI.Get(),0),120.f);
        }
        else if (Step==10) { if (!UI->IsInteractive()) return false; Click(UI.Get(),TEXT("Btn_Cabinet")); }
        else if (Step==11)
        {
            if (UI->IsChangingStage()) return false;
            if (auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget())) { OriginalSize=Window->GetSizeInScreen(); Window->Resize(FVector2D(1000,900)); }
        }
        else if (Step==12)
        {
            if (!UI->IsInteractive()) return false;
            Layout(Test,UI.Get());
            UI->ReportSuccess(); Test->TestFalse(TEXT("Wrong angles cannot report victory"),UI->HasReportedSuccess());
            Press(UI.Get(),FVector2D(.02,.02),Point); Release(Point);
            Test->TestEqual(TEXT("Board corners cannot select a disk"),ReadInt(UI.Get(),TEXT("ActiveRing")),-1);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(bStandalone ? TEXT("DisksIntegration/disks-standalone.png") : TEXT("DisksIntegration/disks-compact.png")),true,false);
        }
        else if (Step==13)
        {
            if (!bStandalone) if (auto Window=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget())) Window->Resize(OriginalSize);
        }
        else if (Step==14)
        {
            if (!bHeld)
            {
                const FVector2D Fractions[]={FVector2D(.5,.42),FVector2D(.5,.24),FVector2D(.5,.07)};
                REQUIRE_DISKS(Press(UI.Get(),Fractions[Ring],Point),TEXT("Disk hold begins")); bHeld=true;
                Test->TestEqual(TEXT("Scaled pointer selects expected ring"),ReadInt(UI.Get(),TEXT("ActiveRing")),Ring);
                Next=Now+.02; return false;
            }
            if (Angle(UI.Get(),Ring)<354) { Next=Now+.01; return false; }
            Release(Point); bHeld=false;
            Test->TestEqual(TEXT("Mouse release stops rotation"),ReadInt(UI.Get(),TEXT("ActiveRing")),-1);
            if (Ring<2) { Test->TestFalse(TEXT("Partial image is not a win"),UI->HasReportedSuccess()); Ring++; Next=Now+.1; return false; }
            REQUIRE_DISKS(UI->HasReportedSuccess(),TEXT("Three aligned disks report success"));
            if (!bStandalone) Test->TestTrue(TEXT("Victory stored on cabinet before visual close"),Source->IsComplete());
        }
        else if (Step==15)
        {
            for (int32 I=0;I<3;++I) Test->TestTrue(TEXT("Soft finishing snap aligns the image"),FMath::Abs(Angle(UI.Get(),I))<.01f);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(bStandalone ? TEXT("DisksIntegration/disks-standalone-solved.png") : TEXT("DisksIntegration/disks-solved.png")),true,false);
            Step++; Next=Now+1.7; return false;
        }
        else if (Step==16)
        {
            Test->TestFalse(TEXT("Win closes after original solved delegate and fade"),UI->IsInViewport());
            Test->TestFalse(TEXT("Win restores gameplay"),PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            if (bStandalone) return true;
            Test->TestFalse(TEXT("Completed cabinet cannot reopen"),Source->OpenMinigame(PC->GetPawn()));
            Test->TestFalse(TEXT("Completed cabinet unavailable"),UInspectionBlueprintLibrary::IsInteractionAvailable(Owner.Get()));
            for (FName Name : {TEXT("Ring_Visual"),TEXT("Widget_InteractionPrompt")}) Test->TestTrue(TEXT("Cabinet indicators retired"),SceneComponent(Owner.Get(),Name)->bHiddenInGame);
            OtherOwner=World->SpawnActor<AActor>(Owner->GetClass(),FVector(5000,5000,0),FRotator::ZeroRotator);
            auto* Other=OtherOwner->FindComponentByClass<UMinigameComponent>();
            REQUIRE_DISKS(Other->OpenMinigame(PC->GetPawn()),TEXT("Second cabinet independent"));
            OtherOwner->Destroy(); Test->TestFalse(TEXT("Owner destruction clears session immediately"),Shared->IsOpen() || PC->IsMoveInputIgnored());
            return true;
        }
        Step++; Next=Now+.5; return false;
    }
private:
    FAutomationTestBase* Test;
    bool bStandalone,bHeld=false;
    double Started,Next=0;
    int32 Step=0,Ring=0;
    FVector Near;
    FVector2D Point,OriginalSize;
    TWeakObjectPtr<AActor> Owner,OtherOwner;
    TWeakObjectPtr<UMinigameComponent> Source;
    TWeakObjectPtr<UActorComponent> Interaction;
    TWeakObjectPtr<UDisksMinigameWidget> UI;
    TStrongObjectPtr<UDisksMinigameWidget> Previous;
};
#undef REQUIRE_DISKS
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBedsideDisksTest,"NENS.Interaction.Minigame.BedsideDisks",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBedsideDisksTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<DisksTest::FFlow>(this,false));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOriginalDisksTest,"NENS.Interaction.Minigame.OriginalDisksLevel",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOriginalDisksTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Minigames/Disks/L_Test_Disks"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<DisksTest::FFlow>(this,true));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
