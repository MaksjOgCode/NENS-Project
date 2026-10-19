#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/TextBlock.h"
#include "GameExperienceSubsystem.h"
#include "GamePresentationData.h"
#include "EndGameWidget.h"
#include "ComputerRecoverySubsystem.h"
#include "ComputerRecoveryComponent.h"
#include "InteractionSessionSubsystem.h"
#include "Sound/SoundWave.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest { bool Click(UUserWidget*,FName,FVector2D = FVector2D(.5,.5)); void Escape(); }
namespace PresentationTest
{
UUserWidget* Menu(UWorld* World)
{
    TArray<UUserWidget*> Found; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Found,UMainMenuPresentationWidget::StaticClass(),true);
    return Found.Num()==1 ? Found[0] : nullptr;
}
bool Map(UWorld* W,const TCHAR* Name) { return UGameplayStatics::GetCurrentLevelName(W,true)==Name; }
#define REQUIRE_FLOW(C,M) if (!Test->TestTrue(M,!!(C))) return true
class FFlow : public IAutomationLatentCommand
{
public:
    FFlow(FAutomationTestBase* T):Test(T),Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-Started>80) { Test->AddError(FString::Printf(TEXT("Presentation test timeout at %d"),Step)); return true; }
        if (Now<Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* W=GEditor->PlayWorld; auto* PC=W->GetFirstPlayerController(); if (!PC || !PC->GetLocalPlayer()) return false;
        auto* F=W->GetGameInstance()->GetSubsystem<UGameExperienceSubsystem>();
        auto* P=W->GetGameInstance()->GetSubsystem<UComputerRecoverySubsystem>();
        auto* S=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        switch (Step)
        {
        case 0:
        {
            REQUIRE_FLOW(F && F->GetSettings(),TEXT("Configured persistent presentation"));
            REQUIRE_FLOW(Map(W,TEXT("Lvl_MainMenu_test")) && Menu(W),TEXT("Existing menu starts normally"));
            auto* Audio=F->GetAmbientComponent(); REQUIRE_FLOW(Audio && Audio->IsPlaying(),TEXT("Menu ambience actually playing"));
            Test->TestTrue(TEXT("Menu uses supplied sound"),Audio->Sound==F->GetSettings()->MainMenuAmbient);
            Test->TestTrue(TEXT("2D sound survives travel and UI focus"),Audio->bIgnoreForFlushing && Audio->bIsUISound);
            auto* Wave=CastChecked<USoundWave>(Audio->Sound);
            Audio->Play(Wave->Duration-.3f); Starts=F->GetAmbientStartCount();
            Next=Now+1; ++Step; return false;
        }
        case 1:
            Test->TestTrue(TEXT("Menu loops beyond the end without creating another voice"),F->GetAmbientComponent()->IsPlaying() && F->GetAmbientStartCount()==Starts);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GamePresentation/menu.png"),true,false);
            REQUIRE_FLOW(WiresTest::Click(Menu(W),TEXT("Btn_Play")),TEXT("Real Play button"));
            Test->TestTrue(TEXT("Play starts fade rather than instant travel"),F->IsTransitioning() && Map(W,TEXT("Lvl_MainMenu_test")));
            Test->TestFalse(TEXT("Repeated Play rejected"),F->StartNewGame());
            break;
        case 2:
            if (!Map(W,TEXT("L_MainRooms")) || F->IsTransitioning()) return false;
            REQUIRE_FLOW(F->GetAmbientComponent() && F->GetAmbientComponent()->IsPlaying(),TEXT("Gameplay ambience playing after travel"));
            Test->TestTrue(TEXT("Gameplay uses supplied track"),F->GetAmbientComponent()->Sound==F->GetSettings()->GameplayAmbient);
            Test->TestEqual(TEXT("One new voice for changed environment"),F->GetAmbientStartCount(),Starts+1);
            F->WorldBegan(W); Test->TestEqual(TEXT("Repeated world notification cannot duplicate ambient"),F->GetAmbientStartCount(),Starts+1);
            Test->TestEqual(TEXT("New run empty inventory"),P->GetInventoryCount(TEXT("PunchCard")),0);
            for (TActorIterator<AActor> It(W);It;++It) if (auto* C=It->FindComponentByClass<UComputerRecoveryComponent>()) Computer=C;
            REQUIRE_FLOW(Computer.IsValid(),TEXT("Actual main-map computer configured"));
            REQUIRE_FLOW(Computer->OpenComputer(PC->GetPawn()),TEXT("Actual placed terminal opens"));
            P->CollectItem(TEXT("ending-test-wardrobe"),TEXT("Wardrobe_PunchCard"),TEXT("PunchCard"));
            P->CollectItem(TEXT("ending-test-clock"),TEXT("FloorClock_PunchCard"),TEXT("PunchCard"));
            P->LoadProgramCards();
            P->MarkSystemReady(EComputerSystem::Power,TEXT("ending-wires"));
            P->MarkSystemReady(EComputerSystem::Fuel,TEXT("ending-fuel"));
            P->MarkSystemReady(EComputerSystem::Display,TEXT("ending-display"));
            P->MarkSystemReady(EComputerSystem::Arithmetic,TEXT("ending-arithmetic"));
            REQUIRE_FLOW(P->BeginRestoration(),TEXT("Complete requirements start actual recovery timer"));
            break;
        case 3:
            if (!F->GetEndingWidget() || !F->GetEndingWidget()->IsInteractive() || F->IsTransitioning()) return false;
            REQUIRE_FLOW(P->IsRestored(),TEXT("Ending follows actual restoration"));
            REQUIRE_FLOW(!Computer->GetActiveWidget() && S->IsActive(F->GetEndingWidget()),TEXT("Ending replaces terminal modal cleanly"));
            Test->TestTrue(TEXT("Gameplay blocked behind ending"),PC->IsMoveInputIgnored() && PC->bShowMouseCursor);
            Test->TestEqual(TEXT("English thank you"),CastChecked<UTextBlock>(F->GetEndingWidget()->WidgetTree->FindWidget(TEXT("Txt_Thanks")))->GetText().ToString(),FString(TEXT("Thank you for playing!")));
            WiresTest::Escape(); Test->TestTrue(TEXT("Escape cannot discard final screen"),S->IsActive(F->GetEndingWidget()) && !F->GetEndingWidget()->IsClosing());
            for (FName Name : {TEXT("TxtTitle"),TEXT("Txt_Thanks"),TEXT("Btn_MainMenu"),TEXT("Btn_Quit"),TEXT("MinigameMist"),TEXT("MinigameGradient")})
                Test->TestNotNull(TEXT("Ending presentation"),F->GetEndingWidget()->WidgetTree->FindWidget(Name));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GamePresentation/ending.png"),true,false);
            break;
        case 4:
            REQUIRE_FLOW(WiresTest::Click(F->GetEndingWidget(),TEXT("Btn_MainMenu")),TEXT("Ending Main menu button clicked"));
            Test->TestTrue(TEXT("Return begins smooth fade"),F->IsTransitioning()); break;
        case 5:
            if (!Map(W,TEXT("Lvl_MainMenu_test")) || F->IsTransitioning() || !Menu(W)) return false;
            Test->TestFalse(TEXT("Ending released on map travel"),F->GetEndingWidget() || S->IsOpen() || PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Main menu ambience returns"),F->GetAmbientComponent() && F->GetAmbientComponent()->Sound==F->GetSettings()->MainMenuAmbient);
            REQUIRE_FLOW(WiresTest::Click(Menu(W),TEXT("Btn_Play")),TEXT("New Play after completed game")); break;
        case 6:
            if (!Map(W,TEXT("L_MainRooms")) || F->IsTransitioning()) return false;
            Test->TestFalse(TEXT("New game clears restored flag"),P->IsRestored() || P->IsRestoring());
            Test->TestEqual(TEXT("New game clears systems"),P->GetReadySystemCount(),0);
            Test->TestEqual(TEXT("New game clears cards"),P->GetLoadedCardCount(),0);
            Test->TestEqual(TEXT("New game clears inventory"),P->GetInventoryCount(TEXT("PunchCard")),0);
            Test->TestFalse(TEXT("No stale ending in new run"),F->GetEndingWidget()!=nullptr);
            { auto* A=F->GetAmbientComponent(); A->Play(CastChecked<USoundWave>(A->Sound)->Duration-.3f); }
            Next=Now+1; ++Step; return false;
        case 7:
            Test->TestTrue(TEXT("Gameplay loops past file end"),F->GetAmbientComponent()->IsPlaying());
            return true;
        }
        ++Step; Next=Now+.35; return false;
    }
private:
    FAutomationTestBase* Test; double Started,Next=0; int32 Step=0,Starts=0;
    TWeakObjectPtr<UComputerRecoveryComponent> Computer;
};
class FQuit : public IAutomationLatentCommand
{
public:
    FQuit(FAutomationTestBase* T):Test(T),Start(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        double Now=FPlatformTime::Seconds();
        if (Now-Start>15) { Test->AddError(TEXT("Quit did not end PIE")); return true; }
        if (Clicked) { if (!GEditor->PlayWorld) return true; return false; }
        if (Now-Start<1 || !GEditor->PlayWorld) return false;
        UWorld* W=GEditor->PlayWorld;
        auto* F=W->GetGameInstance()->GetSubsystem<UGameExperienceSubsystem>();
        if (!Test->TestNotNull(TEXT("Menu exists for quit"),Menu(W))) return true;
        WiresTest::Click(Menu(W),TEXT("Btn_Quit"));
        Test->TestTrue(TEXT("Quit fades before closing"),F->IsTransitioning()); Clicked=true; return false;
    }
private: FAutomationTestBase* Test; double Start; bool Clicked=false;
};
#undef REQUIRE_FLOW
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGamePresentationFlow,"NENS.GamePresentation.EndingMenuAndAmbience",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGamePresentationFlow::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/UI/MainMenu/Lvl_MainMenu_test"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<PresentationTest::FFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGamePresentationQuit,"NENS.GamePresentation.QuitButton",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGamePresentationQuit::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/UI/MainMenu/Lvl_MainMenu_test"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<PresentationTest::FQuit>(this)); return true;
}
#endif
