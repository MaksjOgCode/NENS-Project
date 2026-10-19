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
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "MinigameComponent.h"
#include "MinigameWidget.h"
#include "WiresMinigameWidget.h"
#include "InteractionSessionSubsystem.h"
#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "InspectionBlueprintLibrary.h"
#include "SearchableComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace WiresTest
{
bool Invoke(UObject* Object, FName Name, void* Params = nullptr)
{
    if (Object) if (auto* Fn = Object->FindFunction(Name)) { Object->ProcessEvent(Fn, Params); return true; }
    return false;
}
int32 ReadInt(UObject* Object, FName Name)
{
    if (Object) if (auto* P = FindFProperty<FIntProperty>(Object->GetClass(), Name)) return P->GetPropertyValue_InContainer(Object);
    return -999;
}
USceneComponent* SceneComponent(AActor* Owner, FName Name)
{
    TInlineComponentArray<USceneComponent*> Components(Owner);
    for (auto* C : Components) if (C->GetFName() == Name) return C;
    return nullptr;
}
void Escape()
{
    auto& Slate = FSlateApplication::Get();
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
    Slate.ProcessKeyUpEvent(FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
}
bool Click(UUserWidget* Widget, FName Name, FVector2D Fraction = FVector2D(.5,.5))
{
    UWidget* Target = Name.IsNone() ? Widget : Widget->WidgetTree->FindWidget(Name);
    if (!Target || !Target->GetCachedWidget()) return false;
    auto& Slate = FSlateApplication::Get();
    auto Window = Slate.FindWidgetWindow(Target->GetCachedWidget().ToSharedRef());
    if (!Window) return false;
    FGeometry Geometry = Target->GetCachedGeometry();
    if (Geometry.GetLocalSize().IsNearlyZero()) return false;
    FVector2D Point = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * Fraction);
    Slate.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()), false);
    TSet<FKey> Buttons; Buttons.Add(EKeys::LeftMouseButton);
    Slate.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(0, Point, Point, Buttons, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    Buttons.Reset();
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, Buttons, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    return true;
}
void Connect(UUserWidget* Widget, int32 Pair)
{
    Click(Widget, FName(*FString::Printf(TEXT("BtnL%d"), Pair)));
    Click(Widget, FName(*FString::Printf(TEXT("BtnR%d"), Pair)));
}
void CheckPresentation(FAutomationTestBase* Test, UMinigameWidget* UI)
{
    Test->TestNotNull(TEXT("Wires uses its presentation adapter"), Cast<UWiresMinigameWidget>(UI));
    const auto& View = UI->GetCachedGeometry();
    const FVector2D ViewSize = View.GetLocalSize();
    for (FName Name : {TEXT("Canvas_WiresPresentation"), TEXT("Board"), TEXT("TxtTitle"), TEXT("TxtStatus"), TEXT("Text_WiresControls"), TEXT("BtnClose"), TEXT("BtnL0"), TEXT("BtnL1"), TEXT("BtnL2"), TEXT("BtnR0"), TEXT("BtnR1"), TEXT("BtnR2")})
    {
        auto* Part = UI->WidgetTree->FindWidget(Name);
        if (!Test->TestNotNull(TEXT("Presentation element exists: ") + Name.ToString(), Part)) continue;
        const auto& Geometry = Part->GetCachedGeometry();
        const FVector2D Start = View.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector));
        const FVector2D End = View.AbsoluteToLocal(Geometry.LocalToAbsolute(Geometry.GetLocalSize()));
        Test->TestTrue(TEXT("Element fits in viewport: ") + Name.ToString(),
            Geometry.GetLocalSize().X > 0 && Geometry.GetLocalSize().Y > 0 && Start.X >= -1 && Start.Y >= -1 && End.X <= ViewSize.X + 1 && End.Y <= ViewSize.Y + 1);
        if (Name == TEXT("Canvas_WiresPresentation"))
            Test->TestTrue(TEXT("Complete presentation centered on both axes"), ((Start + End) * .5).Equals(ViewSize * .5, 1));
        if (Name == TEXT("Board"))
            Test->TestTrue(TEXT("Art and connector coordinates preserve their aspect ratio"), FMath::IsNearlyEqual((End.X - Start.X) / (End.Y - Start.Y), 1280. / 720., .002));
        if (auto* Text = Cast<UTextBlock>(Part))
            Test->TestTrue(TEXT("Text uses the shared inspection font"), Text->GetFont().FontObject && Text->GetFont().FontObject->GetFName() == TEXT("F_IMFellEnglish"));
    }
    for (FName Name : {TEXT("WiresGradient"), TEXT("WiresMist")})
    {
        auto* Effect = UI->WidgetTree->FindWidget(Name);
        if (!Test->TestNotNull(TEXT("Shared procedural effect exists"), Effect)) continue;
        Test->TestEqual(TEXT("Effects never intercept a connector click"), Effect->GetVisibility(), ESlateVisibility::HitTestInvisible);
        if (Name == TEXT("WiresGradient"))
        {
            const auto& Geometry = Effect->GetCachedGeometry();
            Test->TestTrue(TEXT("Gradient covers entire modal rather than a small black rectangle"),
                Geometry.GetLocalSize().Equals(ViewSize, 1) && View.AbsoluteToLocal(Geometry.LocalToAbsolute(FVector2D::ZeroVector)).IsNearlyZero(1));
        }
    }
    auto* Board = UI->WidgetTree->FindWidget(TEXT("Board"));
    auto* Background = UI->WidgetTree->FindWidget(TEXT("Img_Bg"));
    if (Board && Background)
    {
        const auto& BoardGeometry = Board->GetCachedGeometry();
        const auto& BackgroundGeometry = Background->GetCachedGeometry();
        const FVector2D Start = BoardGeometry.AbsoluteToLocal(BackgroundGeometry.LocalToAbsolute(FVector2D::ZeroVector));
        Test->TestTrue(TEXT("Opaque source padding cropped without shifting the artwork relative to buttons"),
            FMath::IsNearlyEqual(Start.X, 1280 * .092, .1) && FMath::IsNearlyEqual(BackgroundGeometry.GetLocalSize().X, 1280 * .81, .1));
    }
}

class FComputerFlow : public IAutomationLatentCommand
{
public:
    explicit FComputerFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 100) { Test->AddError(TEXT("Computer wires test timed out")); return true; }
        if (bWatch && Computer.IsValid())
        {
            for (FName Name : {FName(TEXT("Widget_InteractionPrompt")), FName(TEXT("Ring_Visual"))})
            {
                auto* Indicator = SceneComponent(Computer.Get(), Name);
                if (!Test->TestTrue(TEXT("Completed computer indicators stay hidden across overlaps"), Indicator && Indicator->bHiddenInGame)) return true;
            }
        }
        if (Now < Next) return false;
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World) { if (Step) { Test->AddError(TEXT("PIE stopped unexpectedly")); return true; } return false; }
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetPawn() || !PC->GetLocalPlayer()) return false;
        auto* Shared = PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
        auto* Inspection = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (Step == 0)
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (auto* C = It->FindComponentByClass<UMinigameComponent>()) { Source = C; Computer = *It; }
                if (auto* C = It->FindComponentByClass<USearchableComponent>())
                    if (C->Scene && C->Scene->GetFName() == TEXT("DA_Inspection_Wardrobe")) Wardrobe = C;
            }
            if (!Test->TestNotNull(TEXT("Configured computer on gameplay map"), Source.Get()) || !Test->TestNotNull(TEXT("Wardrobe for modal exclusion"), Wardrobe.Get())) return true;
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction = C;
            auto* Point = SceneComponent(Computer.Get(), TEXT("Sphere_Interaction"));
            if (!Test->TestNotNull(TEXT("Existing BPC_Interaction"), Interaction.Get()) || !Test->TestNotNull(TEXT("Computer interaction point"), Point)) return true;
            Near = Point->GetComponentLocation(); Near.Z = PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            OriginalWorld = World; OriginalPawn = PC->GetPawn();
            Step++; Next = Now + .5; return false;
        }
        auto* SourceNow = Source.Get();
        if (!SourceNow) { Test->AddError(TEXT("Computer component lost")); return true; }
        if (Step != 1 && SourceNow->GetActiveWidget() && SourceNow->GetActiveWidget()->IsOpening()) return false;
        if ((Step == 4 || Step == 6) && SourceNow->GetActiveWidget()) return false;
        if (Step == 11 && Other.IsValid() && Other->GetActiveWidget()) return false;
        switch (Step)
        {
        case 1:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestNotNull(TEXT("E interaction route opens wires widget"), SourceNow->GetActiveWidget())) return true;
            Test->TestTrue(TEXT("Same world, no level transition"), OriginalWorld == World);
            Test->TestTrue(TEXT("Same player pawn"), OriginalPawn == PC->GetPawn());
            Test->TestTrue(TEXT("Player position preserved"), PC->GetPawn()->GetActorLocation().Equals(Near));
            Test->TestTrue(TEXT("Cursor visible and movement blocked"), PC->bShowMouseCursor && PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Movement stopped"), PC->GetPawn()->GetVelocity().IsNearlyZero());
            Test->TestFalse(TEXT("Second game cannot overlap"), SourceNow->OpenMinigame(PC->GetPawn()));
            Test->TestFalse(TEXT("Inspection cannot overlap game"), Inspection->Open(Wardrobe.Get()));
            Test->TestFalse(TEXT("Gameplay interactions blocked"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            PreviousAttempt.Reset(SourceNow->GetActiveWidget());
            Test->TestEqual(TEXT("Opening starts transparent"), SourceNow->GetActiveWidget()->GetRenderOpacity(), 0.f);
            if (auto Window = FSlateApplication::Get().FindWidgetWindow(SourceNow->GetActiveWidget()->TakeWidget()))
            { OriginalWindowSize = Window->GetSizeInScreen(); Window->Resize(FVector2D(1700,950)); }
            break;
        case 2:
            if (PreviewStep < 7)
            {
                auto* UI = SourceNow->GetActiveWidget();
                auto Window = FSlateApplication::Get().FindWidgetWindow(UI->TakeWidget());
                if (PreviewStep == 0)
                {
                    CheckPresentation(Test, UI);
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("WiresPresentation/wires-wide.png"), true, false);
                    PreviewStep++; Next = Now + 1.2; return false;
                }
                if (PreviewStep == 1)
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("WiresPresentation/wires-mist-drift.png"), true, false);
                    PreviewStep++; Next = Now + .2; return false;
                }
                if (PreviewStep == 2) { if (Window) Window->Resize(FVector2D(1000,900)); }
                if (PreviewStep == 3)
                {
                    CheckPresentation(Test, UI);
                    Click(UI, TEXT("BtnL0")); Click(UI, TEXT("BtnR1"));
                    Test->TestEqual(TEXT("Real connector clicks still reach Blueprint after compact resize"), ReadInt(UI, TEXT("ConnectedCount")), 0);
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("WiresPresentation/wires-compact.png"), true, false);
                }
                if (PreviewStep == 4) { if (Window) Window->Resize(FVector2D(2000,850)); }
                if (PreviewStep == 5)
                {
                    CheckPresentation(Test, UI);
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("WiresPresentation/wires-ultrawide.png"), true, false);
                }
                if (PreviewStep == 6) { if (Window) Window->Resize(OriginalWindowSize); }
                PreviewStep++; Next = Now + .4; return false;
            }
            Click(SourceNow->GetActiveWidget(), TEXT("BtnL0")); Click(SourceNow->GetActiveWidget(), TEXT("BtnR1"));
            Test->TestEqual(TEXT("Wrong pair is not counted"), ReadInt(SourceNow->GetActiveWidget(), TEXT("ConnectedCount")), 0);
            Test->TestFalse(TEXT("Wrong pair does not complete computer"), SourceNow->IsComplete());
            Connect(SourceNow->GetActiveWidget(), 0);
            Test->TestEqual(TEXT("Correct pair is counted"), ReadInt(SourceNow->GetActiveWidget(), TEXT("ConnectedCount")), 1);
            break;
        case 3:
            Click(SourceNow->GetActiveWidget(), NAME_None, FVector2D(.1,.1)); Escape();
            Test->TestTrue(TEXT("Escape starts fade and retains ownership"), Shared->IsOpen() && SourceNow->GetActiveWidget()->IsClosing());
            Test->TestFalse(TEXT("Abandoned game is not complete"), SourceNow->IsComplete());
            Test->TestTrue(TEXT("Exit fade keeps gameplay input locked"), PC->IsMoveInputIgnored() && PC->bShowMouseCursor);
            break;
        case 4:
            Test->TestFalse(TEXT("Abort restores movement and cursor after fade"), PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestNotNull(TEXT("Abandoned computer can reopen"), SourceNow->GetActiveWidget())) return true;
            Test->TestTrue(TEXT("Fresh attempt has new widget"), SourceNow->GetActiveWidget() != PreviousAttempt.Get());
            Test->TestEqual(TEXT("Fresh attempt resets connected count"), ReadInt(SourceNow->GetActiveWidget(), TEXT("ConnectedCount")), 0);
            PreviousAttempt->RequestClose(); PreviousAttempt->ReportSuccess();
            Test->TestTrue(TEXT("Old callbacks cannot close new attempt"), Shared->IsActive(SourceNow->GetActiveWidget()));
            Test->TestFalse(TEXT("Old callbacks cannot complete new attempt"), SourceNow->IsComplete());
            break;
        case 5:
            Test->TestTrue(TEXT("Original Close button clickable"), Click(SourceNow->GetActiveWidget(), TEXT("BtnClose")));
            Test->TestTrue(TEXT("Original Close button starts exit fade"), Shared->IsOpen() && SourceNow->GetActiveWidget()->IsClosing());
            break;
        case 6:
            Test->TestTrue(TEXT("Inspection opens after aborted minigame"), Inspection->Open(Wardrobe.Get()));
            Test->TestFalse(TEXT("Minigame cannot overlap inspection"), SourceNow->OpenMinigame(PC->GetPawn()));
            break;
        case 7:
            if (Inspection->IsOpen())
            {
                if (!Inspection->GetActiveWidget()->IsClosing())
                {
                    Escape();
                    Test->TestTrue(TEXT("Inspection exit fade retains shared modal ownership"), Shared->IsOpen());
                    Test->TestFalse(TEXT("Minigame cannot overlap a closing inspection"), SourceNow->OpenMinigame(PC->GetPawn()));
                }
                Next = Now + .05; return false;
            }
            Test->TestFalse(TEXT("Inspection Escape restores shared session"), Shared->IsOpen());
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestNotNull(TEXT("Computer opens after inspection"), SourceNow->GetActiveWidget())) return true;
            break;
        case 8:
            for (int32 Pair = 0; Pair < 3; ++Pair) Connect(SourceNow->GetActiveWidget(), Pair);
            Test->TestEqual(TEXT("Updated puzzle records three connected pairs"), ReadInt(SourceNow->GetActiveWidget(), TEXT("ConnectedCount")), 3);
            if (!Test->TestTrue(TEXT("Win reports success to computer"), SourceNow->IsComplete())) return true;
            Test->TestTrue(TEXT("Success screen remains for original delay"), Shared->IsOpen());
            PreviousAttempt.Reset(SourceNow->GetActiveWidget());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("WiresIntegration/wires-win.png"), true, false);
            break;
        case 9:
            if (SourceNow->GetActiveWidget())
            {
                if (!SourceNow->GetActiveWidget()->IsClosing()) Escape();
                Next = Now + .05; return false;
            }
            Test->TestFalse(TEXT("Successful computer cannot reopen"), SourceNow->OpenMinigame(PC->GetPawn()));
            Test->TestFalse(TEXT("Completed computer unavailable"), UInspectionBlueprintLibrary::IsInteractionAvailable(Computer.Get()));
            bWatch = true;
            OtherActor = World->SpawnActor<AActor>(Computer->GetClass(), FVector(5000,5000,0), FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Second computer spawned"), OtherActor.Get())) return true;
            Other = OtherActor->FindComponentByClass<UMinigameComponent>();
            if (!Test->TestNotNull(TEXT("Second minigame component"), Other.Get())) return true;
            Test->TestFalse(TEXT("Completion independent between computers"), Other->IsComplete());
            if (!Test->TestTrue(TEXT("Second computer opens"), Other->OpenMinigame(PC->GetPawn()))) return true;
            PreviousAttempt->RequestClose();
            PC->GetPawn()->SetActorLocation(Near + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            Next = Now + 1.4; Step++; return false;
        case 10:
            Test->TestTrue(TEXT("Old win delay cannot close another window"), Shared->IsActive(Other->GetActiveWidget()));
            Test->TestTrue(TEXT("Old delay cannot release input of new session"), PC->bShowMouseCursor && PC->IsMoveInputIgnored());
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Click(Other->GetActiveWidget(), TEXT("BtnClose"));
            Invoke(Interaction.Get(), TEXT("UpdateInteractionTarget"));
            if (auto* P = FindFProperty<FObjectProperty>(Interaction->GetClass(), TEXT("CurrentInteractable")))
                Test->TestNotEqual(TEXT("Completed computer excluded from target selection"), P->GetObjectPropertyValue_InContainer(Interaction.Get()), static_cast<UObject*>(Computer.Get()));
            { struct {bool Visible = true;} Args; Invoke(Computer.Get(), TEXT("SetAwarenessVisible"), &Args); }
            break;
        case 11:
            PC->GetPawn()->SetActorLocation(Near + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            Test->TestFalse(TEXT("Computer mesh actor is not hidden"), Computer->IsHidden());
            Test->TestTrue(TEXT("Other computer still usable"), Other->OpenMinigame(PC->GetPawn()));
            break;
        case 12:
            OtherActor->Destroy();
            Test->TestFalse(TEXT("Owner destruction closes modal"), Shared->IsOpen());
            Test->TestFalse(TEXT("Owner destruction restores controls"), PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            break;
        case 13:
            Test->TestTrue(TEXT("Completed computer stays complete"), SourceNow->IsComplete());
            Test->TestTrue(TEXT("Gameplay input restored"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            return true;
        }
        Step++; Next = Now + .3; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    bool bWatch = false;
    FVector Near;
    FVector2D OriginalWindowSize = FVector2D::ZeroVector;
    int32 PreviewStep = 0;
    TWeakObjectPtr<UMinigameComponent> Source, Other;
    TWeakObjectPtr<USearchableComponent> Wardrobe;
    TWeakObjectPtr<AActor> Computer, OtherActor;
    TWeakObjectPtr<UActorComponent> Interaction;
    TWeakObjectPtr<UWorld> OriginalWorld;
    TWeakObjectPtr<APawn> OriginalPawn;
    TStrongObjectPtr<UMinigameWidget> PreviousAttempt;
};

class FStandaloneFlow : public IAutomationLatentCommand
{
public:
    explicit FStandaloneFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        double Now = FPlatformTime::Seconds();
        if (Now - Started > 45) { Test->AddError(TEXT("Standalone wires test timed out")); return true; }
        if (Now < Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer()) return false;
        if (Step == 0)
        {
            TArray<UUserWidget*> Widgets; UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UMinigameWidget::StaticClass(), true);
            if (Widgets.IsEmpty()) return false;
            Widget = Cast<UMinigameWidget>(Widgets[0]);
            Test->TestFalse(TEXT("Original test map does not require embedded session"), PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>()->IsOpen());
        }
        else if (Step == 1)
        {
            for (int32 Pair = 0; Pair < 3; ++Pair) Connect(Widget.Get(), Pair);
            Test->TestEqual(TEXT("Standalone updated puzzle can be won"), ReadInt(Widget.Get(), TEXT("ConnectedCount")), 3);
            Test->TestTrue(TEXT("Standalone success reported"), Widget->HasReportedSuccess());
            Step++; Next = Now + 1.5; return false;
        }
        else
        {
            Test->TestFalse(TEXT("Original win delay closes standalone widget"), Widget.IsValid() && Widget->IsInViewport());
            Test->TestFalse(TEXT("Standalone close hides cursor"), PC->bShowMouseCursor);
            return true;
        }
        Step++; Next = Now + .4; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    TWeakObjectPtr<UMinigameWidget> Widget;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FComputerWiresTest, "NENS.Interaction.Minigame.ComputerWires", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FComputerWiresTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<WiresTest::FComputerFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStandaloneWiresTest, "NENS.Interaction.Minigame.OriginalTestLevel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStandaloneWiresTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Minigames/Wires/L_Test_Wires"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<WiresTest::FStandaloneFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
