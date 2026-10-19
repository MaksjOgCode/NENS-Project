#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SphereComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/StrongObjectPtr.h"
#include "ArithmeticMinigameWidget.h"
#include "MinigameComponent.h"
#include "ComputerRecoverySubsystem.h"
#include "InteractionSessionSubsystem.h"
#include "InspectionBlueprintLibrary.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest
{
bool Invoke(UObject*,FName,void* = nullptr);
bool Click(UUserWidget*,FName,FVector2D = FVector2D(.5,.5));
USceneComponent* SceneComponent(AActor*,FName);
void Escape();
}
namespace ArithmeticTest
{
using namespace WiresTest;
#define REQUIRE_MATH(C,M) if (!Test->TestTrue(M,!!(C))) { Previous.Reset(); return true; }
void Key(FKey K)
{
    auto& S=FSlateApplication::Get();
    S.ProcessKeyDownEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));
    S.ProcessKeyUpEvent(FKeyEvent(K,FModifierKeysState(),0,false,0,0));
}
void Type(const TCHAR* Copy)
{
    for (TCHAR C : FString(Copy)) FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(C,FModifierKeysState(),0,false));
}
void Layout(FAutomationTestBase* Test,UArithmeticMinigameWidget* UI)
{
    const auto& View=UI->GetCachedGeometry();
    for (FName Name : {TEXT("Canvas_Minigame"),TEXT("Image_Current"),TEXT("TxtTitle"),TEXT("Txt_Stage"),TEXT("Input_Answer"),TEXT("Btn_Submit"),TEXT("Txt_Feedback"),TEXT("BtnClose")})
    {
        auto* W=UI->WidgetTree->FindWidget(Name); if (!Test->TestNotNull(Name.ToString(),W)) continue;
        const auto& G=W->GetCachedGeometry();
        auto A=View.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));
        auto B=View.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        Test->TestTrue(TEXT("Fits viewport: ")+Name.ToString(),G.GetLocalSize().X>0 && G.GetLocalSize().Y>0 && A.X>=-1 && A.Y>=-1 && B.X<=View.GetLocalSize().X+1 && B.Y<=View.GetLocalSize().Y+1);
        if (auto* T=Cast<UTextBlock>(W)) Test->TestTrue(TEXT("Shared font"),T->GetFont().FontObject && T->GetFont().FontObject->GetFName()==TEXT("F_IMFellEnglish"));
    }
    for (FName Name : {TEXT("MinigameGradient"),TEXT("MinigameMist")}) Test->TestNotNull(TEXT("Shared atmosphere"),UI->WidgetTree->FindWidget(Name));
}
class FFlow : public IAutomationLatentCommand
{
public:
    FFlow(FAutomationTestBase* T):Test(T),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>65) { Previous.Reset(); Test->AddError(FString::Printf(TEXT("Arithmetic test timeout at %d"),Step)); return true; }
        if (Now<Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World=GEditor->PlayWorld; auto* PC=World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer() || !PC->GetPawn()) return false;
        auto* Shared=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        auto* Progress=UComputerRecoverySubsystem::From(PC);
        switch (Step)
        {
        case 0:
        {
            auto* Class=LoadClass<AActor>(nullptr,TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_ComputerCabinet.BP_ComputerCabinet_C"));
            REQUIRE_MATH(Class,TEXT("Computer class exists"));
            Owner=World->SpawnActor<AActor>(Class,FVector(3000,3000,0),FRotator::ZeroRotator);
            REQUIRE_MATH(Owner.IsValid(),TEXT("Computer spawned"));
            Source=Owner->FindComponentByClass<UMinigameComponent>(); REQUIRE_MATH(Source.IsValid(),TEXT("Computer puzzle component"));
            Test->TestEqual(TEXT("Computer now verifies arithmetic"),Source->RepairedSystem,EComputerSystem::Arithmetic);
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction=C;
            auto* Sphere=Cast<USphereComponent>(SceneComponent(Owner.Get(),TEXT("Sphere_Interaction")));
            REQUIRE_MATH(Sphere && Interaction.IsValid(),TEXT("Existing E interaction route"));
            Near=Sphere->GetComponentLocation()+FVector(Sphere->GetScaledSphereRadius()*.7,0,0); Near.Z=PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near,false,nullptr,ETeleportType::TeleportPhysics);
            break;
        }
        case 1:
            Invoke(Interaction.Get(),TEXT("UpdateInteractionTarget")); Invoke(Interaction.Get(),TEXT("TryInteract"));
            UI=Cast<UArithmeticMinigameWidget>(Source->GetActiveWidget()); REQUIRE_MATH(UI.IsValid(),TEXT("E opens arithmetic inside ring"));
            Test->TestTrue(TEXT("Entry starts transparent"),UI->IsOpening() && UI->GetRenderOpacity()==0);
            REQUIRE_MATH(UI->Puzzle,TEXT("Definition hard reference persisted"));
            Input=Cast<UEditableTextBox>(UI->WidgetTree->FindWidget(TEXT("Input_Answer")));
            REQUIRE_MATH(Input.IsValid(),TEXT("Answer field exists"));
            Input->SetText(FText::FromString(TEXT("29"))); UI->SubmitAnswer(); UI->ReportSuccess();
            Test->TestFalse(TEXT("Early submission cannot win"),Source->IsComplete());
            Input->SetText(FText::GetEmpty());
            Test->TestTrue(TEXT("Same room and locked player"),PC->IsMoveInputIgnored() && PC->GetPawn()->GetActorLocation().Equals(Near));
            break;
        case 2:
        {
            const float Blend=UI->WidgetTree->FindWidget(TEXT("Image_Incoming"))->GetRenderOpacity();
            if (!(Blend>0 && Blend<1)) return false;
            Test->TestFalse(TEXT("Input blocked during crossfade"),UI->IsAnswerReady());
            Test->TestEqual(TEXT("First frame still current during blend"),UI->GetDisplayedFrame(),0);
            Previous.Reset(UI.Get()); Escape(); Test->TestTrue(TEXT("Can close during frames"),UI->IsClosing());
            break;
        }
        case 3:
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Abort restores movement"),Shared->IsOpen() || PC->IsMoveInputIgnored());
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Cast<UArithmeticMinigameWidget>(Source->GetActiveWidget());
            REQUIRE_MATH(UI.IsValid(),TEXT("Computer reopens"));
            Test->TestEqual(TEXT("Reopen resets sequence"),UI->GetDisplayedFrame(),0);
            Previous->SubmitAnswer(); Previous->ReportSuccess(); Previous->RequestClose(); Previous.Reset();
            Test->TestTrue(TEXT("Stale UI cannot close new window"),Shared->IsActive(UI.Get()));
            Test->TestFalse(TEXT("Stale UI cannot finish puzzle"),Source->IsComplete());
            break;
        case 4:
            if (!UI->IsAnswerReady()) return false;
            Test->TestEqual(TEXT("Only final frame enables answer"),UI->GetDisplayedFrame(),3);
            Input=Cast<UEditableTextBox>(UI->WidgetTree->FindWidget(TEXT("Input_Answer")));
            Test->TestTrue(TEXT("Input has keyboard focus"),Input->HasKeyboardFocus());
            Layout(Test,UI.Get());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ArithmeticPuzzle/ready.png"),true,false);
            break;
        case 5:
            Type(TEXT("28")); Key(EKeys::Enter);
            Test->TestEqual(TEXT("Real keyboard reaches answer field"),Input->GetText().ToString(),FString(TEXT("28")));
            Test->TestFalse(TEXT("Wrong answer keeps puzzle active"),Source->IsComplete());
            Test->TestTrue(TEXT("Retry copy visible"),CastChecked<UTextBlock>(UI->WidgetTree->FindWidget(TEXT("Txt_Feedback")))->GetText().ToString().Contains(TEXT("try again")));
            break;
        case 6:
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ArithmeticPuzzle/wrong.png"),true,false);
            Escape(); Test->TestTrue(TEXT("Escape works with text focus"),UI->IsClosing());
            break;
        case 7:
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Focused exit restores controls"),Shared->IsOpen() || PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Invoke(Interaction.Get(),TEXT("TryInteract")); UI=Cast<UArithmeticMinigameWidget>(Source->GetActiveWidget());
            REQUIRE_MATH(UI.IsValid(),TEXT("Retry after exit"));
            if (auto W=FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget())) { OriginalSize=W->GetSizeInScreen(); W->Resize(FVector2D(1000,900)); }
            break;
        case 8:
            if (!UI->IsAnswerReady()) return false;
            Layout(Test,UI.Get()); FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ArithmeticPuzzle/compact.png"),true,false);
            break;
        case 9:
            Type(TEXT("29")); Key(EKeys::Enter);
            REQUIRE_MATH(Source->IsComplete(),TEXT("Real typed 29 solves puzzle"));
            Test->TestTrue(TEXT("Arithmetic restored globally"),Progress->IsSystemReady(EComputerSystem::Arithmetic));
            Test->TestFalse(TEXT("Arithmetic does not replace electricity"),Progress->IsSystemReady(EComputerSystem::Power));
            Test->TestFalse(TEXT("One check cannot restore whole computer"),Progress->CanRestore());
            UI->SubmitAnswer(); Test->TestEqual(TEXT("Repeat submit counts once"),Progress->GetReadySystemCount(),1);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ArithmeticPuzzle/success.png"),true,false);
            break;
        case 10:
            if (Source->GetActiveWidget()) return false;
            Test->TestFalse(TEXT("Success closes and releases controls"),Shared->IsOpen() || PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Test->TestFalse(TEXT("Completed computer cannot reopen"),Source->OpenMinigame(PC->GetPawn()));
            Test->TestFalse(TEXT("Completed computer removed from targets"),UInspectionBlueprintLibrary::IsInteractionAvailable(Owner.Get()));
            if (auto W=FSlateApplication::Get().GetActiveTopLevelWindow()) W->Resize(OriginalSize);
            return true;
        }
        ++Step; Next=Now+.25; return false;
    }
private:
    FAutomationTestBase* Test; double Started,Next=0; int32 Step=0; FVector Near; FVector2D OriginalSize;
    TWeakObjectPtr<AActor> Owner; TWeakObjectPtr<UMinigameComponent> Source;
    TWeakObjectPtr<UArithmeticMinigameWidget> UI; TWeakObjectPtr<UEditableTextBox> Input;
    TWeakObjectPtr<UActorComponent> Interaction; TStrongObjectPtr<UArithmeticMinigameWidget> Previous;
};
#undef REQUIRE_MATH
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArithmeticFlowTest,"NENS.Arithmetic.PIEFlow",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArithmeticFlowTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ArithmeticTest::FFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
