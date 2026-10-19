#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "ClockInspectionComponent.h"
#include "ComputerRecoverySubsystem.h"
#include "ClockInspectionWidget.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"
#include "InspectionBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockModelTest, "NENS.Inspection.Clock.ProgressAndValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockModelTest::RunTest(const FString&)
{
    auto* Puzzle = LoadObject<UClockPuzzleData>(nullptr, TEXT("/Game/NENS/UI/Inspection/DA_ClockPuzzle_FloorClock.DA_ClockPuzzle_FloorClock"));
    auto* Scene = LoadObject<UInspectionSceneData>(nullptr, TEXT("/Game/NENS/UI/Inspection/DA_Inspection_FloorClock.DA_Inspection_FloorClock"));
    if (!TestNotNull(TEXT("Clock puzzle data"), Puzzle) || !TestNotNull(TEXT("Clock compartment data"), Scene)) return false;
    auto* Source = NewObject<UClockInspectionComponent>(); Source->Puzzle = Puzzle; Source->Scene = Scene;
    FString Error;
    TestTrue(TEXT("Configured clock validates"), Source->ValidateSetup(Error));
    USearchableComponent* ThroughBase = Source;
    const FName ID = Scene->Pickups[0].PickupID;
    TestFalse(TEXT("Even base-class collection cannot bypass unsolved clock"), ThroughBase->TryCollect(ID));
    TestFalse(TEXT("Unplaced hand cannot rotate"), Source->RotateHand(EClockHand::Short, 0));
    TestTrue(TEXT("Short hand placed"), Source->PlaceHand(EClockHand::Short));
    TestFalse(TEXT("Same hand cannot be installed twice"), Source->PlaceHand(EClockHand::Short));
    TestTrue(TEXT("Long hand placed"), Source->PlaceHand(EClockHand::Long));
    TestFalse(TEXT("Placing hands is not enough to solve"), Source->IsPuzzleSolved());
    TestFalse(TEXT("NaN rotation rejected"), Source->RotateHand(EClockHand::Long, std::numeric_limits<float>::quiet_NaN()));
    Source->RotateHand(EClockHand::Short, Puzzle->ShortHand.TargetAngle + 360);
    Source->RotateHand(EClockHand::Long, Puzzle->LongHand.TargetAngle - 360);
    TestTrue(TEXT("Both correct angles solve, including wraparound"), Source->IsPuzzleSolved());
    TestFalse(TEXT("Solved mechanism is not yet a collected clock"), Source->IsComplete());
    TestFalse(TEXT("Solved hand cannot be disturbed"), Source->RotateHand(EClockHand::Long, 0));
    TestTrue(TEXT("Unlocked card can be collected"), ThroughBase->TryCollect(ID));
    TestTrue(TEXT("Card completes clock"), Source->IsComplete());
    TestFalse(TEXT("No duplicate card"), ThroughBase->TryCollect(ID));
    auto* Other = NewObject<UClockInspectionComponent>(); Other->Puzzle = Puzzle; Other->Scene = Scene;
    TestFalse(TEXT("Another clock has independent puzzle progress"), Other->IsPuzzleSolved());
    TestFalse(TEXT("Another clock has independent card progress"), Other->IsCollected(ID));
    auto* Invalid = DuplicateObject<UClockPuzzleData>(Puzzle, GetTransientPackage());
    Invalid->SnapDegrees = 0;
    TestFalse(TEXT("Invalid rotation settings rejected"), Invalid->Validate(Error));
    Invalid->SnapDegrees = 6; Invalid->LongHand.UVMax = Invalid->LongHand.UVMin;
    TestFalse(TEXT("Empty hand art region rejected"), Invalid->Validate(Error));
    return true;
}

namespace ClockTest
{
bool Invoke(UObject* Object, FName Name)
{
    if (Object) if (auto* Fn = Object->FindFunction(Name)) { Object->ProcessEvent(Fn, nullptr); return true; }
    return false;
}
USceneComponent* Component(AActor* Actor, FName Name)
{
    TInlineComponentArray<USceneComponent*> All(Actor);
    for (auto* C : All) if (C->GetFName() == Name) return C;
    return nullptr;
}
FVector2D Direction(float Angle)
{
    return FVector2D(FMath::Sin(FMath::DegreesToRadians(Angle)), -FMath::Cos(FMath::DegreesToRadians(Angle)));
}
void Key(FKey K)
{
    auto& S = FSlateApplication::Get();
    S.ProcessKeyDownEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
    S.ProcessKeyUpEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
}
bool Mouse(UUserWidget* Widget, FVector2D From, FVector2D To)
{
    auto& S = FSlateApplication::Get();
    auto Cached = Widget->GetCachedWidget();
    if (!Cached) return false;
    auto Window = S.FindWidgetWindow(Cached.ToSharedRef());
    if (!Window) return false;
    TSet<FKey> None, Pressed; Pressed.Add(EKeys::LeftMouseButton);
    S.ProcessMouseMoveEvent(FPointerEvent(0, From, From, None, EKeys::Invalid, 0, FModifierKeysState()), false);
    S.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(0, From, From, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    S.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed, EKeys::Invalid, 0, FModifierKeysState()), false);
    S.ProcessMouseButtonUpEvent(FPointerEvent(0, To, To, None, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    return true;
}
bool Drag(UClockInspectionWidget* Widget, FVector2D From, FVector2D To)
{
    const auto& G = Widget->GetSceneCanvas()->GetCachedGeometry();
    return Mouse(Widget, G.LocalToAbsolute(From), G.LocalToAbsolute(To));
}
void BeginUnfinishedDrag(UClockInspectionWidget* Widget, FVector2D From, FVector2D To)
{
    auto& S = FSlateApplication::Get();
    auto Window = S.FindWidgetWindow(Widget->GetCachedWidget().ToSharedRef());
    const auto& G = Widget->GetSceneCanvas()->GetCachedGeometry();
    From = G.LocalToAbsolute(From); To = G.LocalToAbsolute(To);
    TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
    S.ProcessMouseMoveEvent(FPointerEvent(0, From, From, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()), false);
    S.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(0, From, From, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    S.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed, EKeys::Invalid, 0, FModifierKeysState()), false);
}
void Place(UClockInspectionWidget* Widget, UClockInspectionComponent* Source, EClockHand Hand, bool Valid = true)
{
    Drag(Widget, (Widget->HandAnchor(Hand) + Widget->HandTip(Hand)) * .5,
        Valid ? Source->Puzzle->DialCenter : FVector2D(100,100));
}
void Rotate(UClockInspectionWidget* Widget, UClockInspectionComponent* Source, EClockHand Hand, float Angle)
{
    Widget->SelectHand(Hand);
    Drag(Widget, Widget->HandTip(Hand), Source->Puzzle->DialCenter + Direction(Angle) * Source->Puzzle->Hand(Hand).TipLength);
}
class FClockFlow : public IAutomationLatentCommand
{
public:
    explicit FClockFlow(FAutomationTestBase* T) : Test(T), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 100) { Test->AddError(TEXT("Clock UI flow timed out")); return true; }
        if (bWatch && Actor.IsValid())
            for (FName Name : {FName(TEXT("Ring_Visual")), FName(TEXT("Widget_InteractionPrompt"))})
            {
                auto* C = Component(Actor.Get(), Name);
                if (!Test->TestTrue(TEXT("Collected clock indicators stay hidden across overlaps"), C && C->bHiddenInGame)) return true;
            }
        if (Now < Next) return false;
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World) { if (Step) { Test->AddError(TEXT("PIE closed unexpectedly")); return true; } return false; }
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetPawn() || !PC->GetLocalPlayer()) return false;
        auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (Step == 0)
        {
            for (TActorIterator<AActor> It(World); It; ++It) if (auto* C = It->FindComponentByClass<UClockInspectionComponent>()) { Source = C; Actor = *It; break; }
            if (!Test->TestNotNull(TEXT("Configured clock on gameplay map"), Source.Get())) return true;
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction = C;
            auto* Point = Component(Actor.Get(), TEXT("Sphere_Interaction"));
            if (!Test->TestNotNull(TEXT("Clock interaction point"), Point)) return true;
            Near = Point->GetComponentLocation(); Near.Z = PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Step++; Next = Now + .5; return false;
        }
        auto* C = Source.Get();
        auto* UI = Cast<UClockInspectionWidget>(Session->GetActiveWidget());
        if (UI && (UI->IsOpening() || UI->IsClosing())) return false;
        switch (Step)
        {
        case 1:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            UI = Cast<UClockInspectionWidget>(Session->GetActiveWidget());
            if (!Test->TestNotNull(TEXT("E route opens clock widget"), UI)) return true;
            Test->TestTrue(TEXT("Clock modal shows cursor and blocks movement"), PC->bShowMouseCursor && PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Unsolved clock card cannot be collected"), UI->CollectPickup(TEXT("FloorClock_PunchCard")));
            OldWidget.Reset(UI);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("ClockPuzzle/clock-puzzle.png"), true, false);
            break;
        case 2:
            if (!Test->TestNotNull(TEXT("Puzzle stays open"), UI)) return true;
            Place(UI, C, EClockHand::Short, false);
            Test->TestFalse(TEXT("Drop outside dial returns hand to tray"), C->IsHandPlaced(EClockHand::Short));
            Place(UI, C, EClockHand::Short);
            if (!Test->TestTrue(TEXT("Actual drag places short hand"), C->IsHandPlaced(EClockHand::Short))) return true;
            break;
        case 3:
            Rotate(UI, C, EClockHand::Short, 90);
            Test->TestTrue(TEXT("Actual drag rotates installed hand"), FMath::Abs(FMath::FindDeltaAngleDegrees(C->GetHandAngle(EClockHand::Short), 90.f)) < 1);
            Key(EKeys::Right);
            Test->TestTrue(TEXT("Keyboard fine adjustment works"), FMath::Abs(FMath::FindDeltaAngleDegrees(C->GetHandAngle(EClockHand::Short), 96.f)) < 1);
            { auto& S = FSlateApplication::Get(); const FVector2D P = UI->GetSceneCanvas()->GetCachedGeometry().LocalToAbsolute(UI->HandTip(EClockHand::Short));
              S.ProcessMouseWheelOrGestureEvent(FPointerEvent(0, P, P, TSet<FKey>(), EKeys::Invalid, -1, FModifierKeysState()), nullptr); }
            SavedAngle = C->GetHandAngle(EClockHand::Short);
            Test->TestTrue(TEXT("Actual mouse wheel fine adjustment works"), FMath::Abs(FMath::FindDeltaAngleDegrees(SavedAngle, 90.f)) < 1);
            Key(EKeys::Tab); Test->TestEqual(TEXT("Tab selects other hand"), UI->GetSelectedHand(), EClockHand::Long); Key(EKeys::Tab);
            BeginUnfinishedDrag(UI, UI->HandTip(EClockHand::Short), C->Puzzle->DialCenter + Direction(150) * 70);
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("Escape before solution starts UI exit fade"), UI->IsClosing());
            Test->TestTrue(TEXT("Exit fade keeps movement blocked and cursor visible"), PC->IsMoveInputIgnored() && PC->bShowMouseCursor);
            Test->TestFalse(TEXT("Escape releases an unfinished mouse capture"), UI->HasMouseCapture());
            { auto& S = FSlateApplication::Get(); const FVector2D P = S.GetCursorPos();
              S.ProcessMouseButtonUpEvent(FPointerEvent(0, P, P, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState())); }
            break;
        case 4:
            Test->TestFalse(TEXT("Abort restores movement and cursor after exit fade"), PC->IsMoveInputIgnored() || PC->bShowMouseCursor);
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            UI = Cast<UClockInspectionWidget>(Session->GetActiveWidget());
            if (!Test->TestNotNull(TEXT("Unsolved clock reopens"), UI)) return true;
            Test->TestTrue(TEXT("Placement survives closing"), C->IsHandPlaced(EClockHand::Short));
            Test->TestEqual(TEXT("Rotation survives closing"), C->GetHandAngle(EClockHand::Short), SavedAngle);
            Test->TestFalse(TEXT("Old widget cannot collect in new attempt"), OldWidget->CollectPickup(TEXT("FloorClock_PunchCard")));
            break;
        case 5:
            Place(UI, C, EClockHand::Long);
            if (!Test->TestTrue(TEXT("Actual drag places long hand"), C->IsHandPlaced(EClockHand::Long))) return true;
            Rotate(UI, C, EClockHand::Short, C->Puzzle->ShortHand.TargetAngle);
            Test->TestFalse(TEXT("One correct hand is insufficient"), C->IsPuzzleSolved());
            break;
        case 6:
            Rotate(UI, C, EClockHand::Long, C->Puzzle->LongHand.TargetAngle);
            if (!Test->TestTrue(TEXT("Both correctly rotated hands solve clock"), C->IsPuzzleSolved())) return true;
            Test->TestTrue(TEXT("Transition starts within existing session"), UI->IsTransitioning());
            Test->TestFalse(TEXT("Solved puzzle is not yet collected"), C->IsComplete());
            break;
        case 7:
            Test->TestTrue(TEXT("Soft transition is in progress"), UI->IsTransitioning());
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("Escape during compartment transition starts UI exit fade"), UI->IsClosing());
            Test->TestTrue(TEXT("Solution committed before fade completes"), C->IsPuzzleSolved());
            break;
        case 8:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            UI = Cast<UClockInspectionWidget>(Session->GetActiveWidget());
            if (!Test->TestNotNull(TEXT("Solved clock can reopen"), UI)) return true;
            Test->TestTrue(TEXT("Re-entry opens compartment directly"), UI->IsCompartmentVisible());
            Test->TestEqual(TEXT("Uncollected card remains visible"), UI->GetPickupButtons()[0]->GetVisibility(), ESlateVisibility::Visible);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("ClockPuzzle/clock-compartment.png"), true, false);
            break;
        case 9:
            Key(EKeys::Escape);
            Test->TestTrue(TEXT("Uncollected compartment stays available"), UInspectionBlueprintLibrary::IsInteractionAvailable(Actor.Get()));
            break;
        case 10:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            UI = Cast<UClockInspectionWidget>(Session->GetActiveWidget());
            if (!Test->TestTrue(TEXT("Compartment survives another exit"), UI && UI->IsCompartmentVisible())) return true;
            break;
        case 11:
        {
            auto* Button = UI->GetPickupButtons()[0].Get();
            const FVector2D Point = Button->GetCachedGeometry().LocalToAbsolute(Button->GetCachedGeometry().GetLocalSize() * .5);
            Mouse(UI, Point, Point);
            if (!Test->TestTrue(TEXT("Real click collects clock punch card"), C->IsComplete())) return true;
            Test->TestEqual(TEXT("Clock pickup click adds card to current run bag"),UComputerRecoverySubsystem::From(C)->GetInventoryCount(TEXT("PunchCard")),1);
            Test->TestFalse(TEXT("Duplicate card rejected"), C->TryCollect(TEXT("FloorClock_PunchCard")));
            Test->TestTrue(TEXT("Collecting card keeps screen open until Escape"), Session->IsOpen());
            Key(EKeys::Escape);
            PC->GetPawn()->SetActorLocation(Near + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            break;
        }
        case 12:
            bWatch = true;
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Test->TestFalse(TEXT("Collected clock unavailable"), UInspectionBlueprintLibrary::IsInteractionAvailable(Actor.Get()));
            Test->TestFalse(TEXT("Collected clock cannot reopen"), Session->Open(C));
            OtherActor = World->SpawnActor<AActor>(Actor->GetClass(), FVector(5000,5000,0), FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Independent second clock"), OtherActor.Get())) return true;
            Other = OtherActor->FindComponentByClass<UClockInspectionComponent>();
            Test->TestFalse(TEXT("Second clock puzzle progress independent"), Other->IsPuzzleSolved());
            if (!Test->TestTrue(TEXT("Second clock opens"), Session->Open(Other.Get()))) return true;
            break;
        case 13:
            Other->PlaceHand(EClockHand::Short); Other->PlaceHand(EClockHand::Long);
            Other->RotateHand(EClockHand::Short, Other->Puzzle->ShortHand.TargetAngle);
            Other->RotateHand(EClockHand::Long, Other->Puzzle->LongHand.TargetAngle);
            break;
        case 14:
            if (!Test->TestTrue(TEXT("Normal uninterrupted transition runs"), UI->IsTransitioning())) return true;
            { auto* BG = Cast<UImage>(UI->WidgetTree->FindWidget(TEXT("Image_Background"))); Test->TestTrue(TEXT("New background fades smoothly"), BG && BG->GetRenderOpacity() > 0 && BG->GetRenderOpacity() < 1); }
            Next = Now + 1; Step++; return false;
        case 15:
            Test->TestTrue(TEXT("Crossfade completes and unlocks pickup UI"), UI->IsCompartmentVisible());
            Test->TestFalse(TEXT("Second clock card still independent"), Other->IsComplete());
            UI->RemoveFromParent();
            break;
        case 16:
            Test->TestFalse(TEXT("External removal also restores shared session"), Session->IsOpen());
            Test->TestFalse(TEXT("No leaked movement lock or cursor"), PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Clock furniture itself remains visible"), Actor->IsHidden());
            OtherActor->Destroy();
            return true;
        }
        Step++; Next = Now + .25; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    float SavedAngle = 0;
    bool bWatch = false;
    FVector Near;
    TWeakObjectPtr<UClockInspectionComponent> Source, Other;
    TWeakObjectPtr<AActor> Actor, OtherActor;
    TWeakObjectPtr<UActorComponent> Interaction;
    TStrongObjectPtr<UClockInspectionWidget> OldWidget;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClockPIETest, "NENS.Inspection.Clock.PIEFlow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FClockPIETest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ClockTest::FClockFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
