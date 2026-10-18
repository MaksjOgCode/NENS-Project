#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "InspectionBlueprintLibrary.h"
#include "SearchableComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace
{
bool Invoke(UObject* Object, FName Name, void* Params = nullptr)
{
    if (!Object) return false;
    if (auto* Function = Object->FindFunction(Name)) { Object->ProcessEvent(Function, Params); return true; }
    return false;
}
USceneComponent* FindSceneComponent(AActor* Owner, FName Name)
{
    TInlineComponentArray<USceneComponent*> Components(Owner);
    for (auto* C : Components) if (C->GetFName() == Name) return C;
    return nullptr;
}
void Key(FKey K)
{
    auto& Slate = FSlateApplication::Get();
    Slate.ProcessKeyDownEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
    Slate.ProcessKeyUpEvent(FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0));
}
bool Click(UWidget* Widget, FVector2D Fraction = FVector2D(.5, .5))
{
    auto SlateWidget = Widget->GetCachedWidget();
    if (!SlateWidget.IsValid()) return false;
    auto& Slate = FSlateApplication::Get();
    auto Window = Slate.FindWidgetWindow(SlateWidget.ToSharedRef());
    if (!Window.IsValid()) return false;
    const FGeometry G = Widget->GetCachedGeometry();
    if (G.GetLocalSize().IsNearlyZero()) return false;
    const FVector2D Point = G.LocalToAbsolute(G.GetLocalSize() * Fraction);
    TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
    Slate.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()), false);
    Slate.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), FPointerEvent(0, Point, Point, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    Pressed.Reset();
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
    return true;
}

class FInspectionFlow : public IAutomationLatentCommand
{
public:
    explicit FInspectionFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    virtual bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 75) { Test->AddError(TEXT("Inspection PIE flow timed out")); return true; }
        if (bWatchCompletedPrompt && Wardrobe.IsValid())
        {
            auto* Prompt = Cast<UWidgetComponent>(FindSceneComponent(Wardrobe.Get(), TEXT("Widget_InteractionPrompt")));
            if (!Test->TestNotNull(TEXT("Completed wardrobe prompt exists"), Prompt)) return true;
            if (!Test->TestFalse(TEXT("Completed E component stays hidden during overlap changes"), Prompt->IsVisible() && !Prompt->bHiddenInGame)) return true;
        }
        if (Now < Next) return false;
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World) { if (Step > 0) { Test->AddError(TEXT("PIE ended unexpectedly (Escape/focus regression)")); return true; } return false; }
        APlayerController* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetPawn() || !PC->GetLocalPlayer()) return false;
        auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (!Session) { Test->AddError(TEXT("Inspection subsystem missing")); return true; }
        if (Step == 0)
        {
            for (TActorIterator<AActor> It(World); It; ++It)
                if (auto* C = It->FindComponentByClass<USearchableComponent>())
                    if (C->Scene && C->Scene->GetFName() == TEXT("DA_Inspection_Wardrobe")) { Source = C; Wardrobe = *It; break; }
            if (!Test->TestNotNull(TEXT("Level contains configured wardrobe"), Source.Get())) return true;
            for (UActorComponent* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction = C;
            if (!Test->TestNotNull(TEXT("Existing BPC_Interaction found"), Interaction.Get())) return true;
            if (auto* Prompt = Cast<UWidgetComponent>(FindSceneComponent(Wardrobe.Get(), TEXT("Widget_InteractionPrompt"))))
            {
                auto* UI = Prompt->GetUserWidgetObject();
                if (!Test->TestNotNull(TEXT("World prompt widget initialized"), UI)) return true;
                auto* Probe = CreateWidget<UUserWidget>(PC, UI->GetClass());
                if (!Test->TestNotNull(TEXT("Prompt state regression fixture"), Probe)) return true;
                struct { bool Focused = false; } Args;
                for (int32 Repeat = 0; Repeat < 3; ++Repeat) Invoke(Probe, TEXT("SetPromptFocused"), &Args);
                Test->TestFalse(TEXT("Repeated hide on an already hidden prompt does not start animation"), Probe->IsAnyAnimationPlaying());
            }
            auto* Point = FindSceneComponent(Wardrobe.Get(), TEXT("InteractionPoint"));
            if (!Test->TestNotNull(TEXT("Wardrobe interaction point"), Point)) return true;
            FVector Position = Point->GetComponentLocation(); Position.Z = PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
            Step++; Next = Now + .5; return false;
        }
        auto* C = Source.Get();
        if (!C) { Test->AddError(TEXT("Source lost during test")); return true; }
        switch (Step)
        {
        case 1:
            Test->TestTrue(TEXT("TryInteract exists"), Invoke(Interaction.Get(), TEXT("TryInteract")));
            if (!Test->TestTrue(TEXT("Existing interaction opens configured UI"), Session->IsOpen())) return true;
            Test->TestTrue(TEXT("Cursor visible"), PC->bShowMouseCursor);
            Test->TestTrue(TEXT("Movement blocked"), PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Movement velocity stopped"), PC->GetPawn()->GetVelocity().IsNearlyZero());
            Test->TestFalse(TEXT("Gameplay interaction blocked during modal"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            Test->TestFalse(TEXT("Second screen rejected"), Session->Open(C));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("InspectionImplementation/inspection-open.png"), true, false);
            break;
        case 2:
            Test->TestTrue(TEXT("Background mouse routed"), Click(Session->GetActiveWidget(), FVector2D(.1,.1)));
            Key(EKeys::Escape);
            break;
        case 3:
            if (!Test->TestFalse(TEXT("Escape after background click closes"), Session->IsOpen())) return true;
            Test->TestFalse(TEXT("Cursor restored"), PC->bShowMouseCursor);
            Test->TestFalse(TEXT("Movement restored"), PC->IsMoveInputIgnored());
            Test->TestFalse(TEXT("Exit without collection keeps object incomplete"), C->IsComplete());
            if (auto* Prompt = Cast<UWidgetComponent>(FindSceneComponent(Wardrobe.Get(), TEXT("Widget_InteractionPrompt")) ))
                Test->TestFalse(TEXT("Unfinished wardrobe keeps its prompt component enabled"), Prompt->bHiddenInGame);
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestTrue(TEXT("Re-enter without collecting"), Session->IsOpen())) return true;
            break;
        case 4:
            if (!Test->TestEqual(TEXT("One card shown"), Session->GetActiveWidget()->GetPickupButtons().Num(), 1)) return true;
            Test->TestTrue(TEXT("Card mouse click routed"), Click(Session->GetActiveWidget()->GetPickupButtons()[0]));
            break;
        case 5:
            if (!Test->TestTrue(TEXT("Real click records collection"), C->IsComplete())) return true;
            Test->TestTrue(TEXT("Last item does not auto-close"), Session->IsOpen());
            Test->TestEqual(TEXT("Card hidden after confirmation"), Session->GetActiveWidget()->GetPickupButtons()[0]->GetVisibility(), ESlateVisibility::Collapsed);
            Test->TestFalse(TEXT("Duplicate collection rejected"), C->TryCollect(C->Scene->Pickups[0].PickupID));
            Key(EKeys::Escape);
            break;
        case 6:
            if (!Test->TestFalse(TEXT("Escape after pickup closes"), Session->IsOpen())) return true;
            Test->TestFalse(TEXT("Completed actor unavailable"), UInspectionBlueprintLibrary::IsInteractionAvailable(Wardrobe.Get()));
            Invoke(Interaction.Get(), TEXT("UpdateInteractionTarget"));
            if (auto* P = FindFProperty<FObjectProperty>(Interaction->GetClass(), TEXT("CurrentInteractable")))
                Test->TestNotEqual(TEXT("Completed actor excluded from selection"), P->GetObjectPropertyValue_InContainer(Interaction.Get()), static_cast<UObject*>(Wardrobe.Get()));
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            Test->TestFalse(TEXT("Completed actor does not reopen"), Session->IsOpen());
            { struct { bool Visible = true; } Args; Invoke(Wardrobe.Get(), TEXT("SetAwarenessVisible"), &Args); }
            break;
        case 7:
            if (auto* P = FindFProperty<FBoolProperty>(Wardrobe->GetClass(), TEXT("bAwarenessVisible")))
                Test->TestFalse(TEXT("Completed awareness cannot re-enable"), P->GetPropertyValue_InContainer(Wardrobe.Get()));
            if (auto* Ring = FindSceneComponent(Wardrobe.Get(), TEXT("Ring_Visual"))) Test->TestFalse(TEXT("Ring hidden after reverse fade"), Ring->IsVisible());
            bWatchCompletedPrompt = true;
            NearPosition = PC->GetPawn()->GetActorLocation();
            PC->GetPawn()->SetActorLocation(NearPosition + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            // Independent second actor, same configuration, two placements of one item type.
            Second = World->SpawnActor<AActor>(Wardrobe->GetClass(), FVector(5000,5000,0), FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Second wardrobe spawned"), Second.Get())) return true;
            Other = Second->FindComponentByClass<USearchableComponent>();
            if (!Test->TestNotNull(TEXT("Second source"), Other.Get())) return true;
            Test->TestFalse(TEXT("Progress independent of first wardrobe"), Other->IsComplete());
            { auto* Data = DuplicateObject<UInspectionSceneData>(C->Scene, Other.Get());
              FInspectionPickup Extra = Data->Pickups[0]; Extra.PickupID = TEXT("SecondCard"); Extra.Position = FVector2D(316,660); Data->Pickups.Add(Extra); Other->Scene = Data; }
            Test->TestTrue(TEXT("Multiple-pickup scene opens"), Session->Open(Other.Get()));
            break;
        case 8:
            PC->GetPawn()->SetActorLocation(NearPosition, false, nullptr, ETeleportType::TeleportPhysics);
            if (!Test->TestTrue(TEXT("Multi scene active"), Session->IsOpen())) return true;
            Test->TestEqual(TEXT("Two pickups shown"), Session->GetActiveWidget()->GetPickupButtons().Num(), 2);
            Click(Session->GetActiveWidget()->GetPickupButtons()[0]);
            Key(EKeys::Escape);
            break;
        case 9:
            PC->GetPawn()->SetActorLocation(NearPosition + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            Test->TestFalse(TEXT("Partially collected scene remains available"), Other->IsComplete());
            if (!Test->TestTrue(TEXT("Partially collected scene reopens"), Session->Open(Other.Get()))) return true;
            Test->TestEqual(TEXT("Collected item does not respawn"), Session->GetActiveWidget()->GetPickupButtons().Num(), 1);
            break;
        case 10:
            PC->GetPawn()->SetActorLocation(NearPosition, false, nullptr, ETeleportType::TeleportPhysics);
            Click(Session->GetActiveWidget()->GetPickupButtons()[0]);
            Test->TestTrue(TEXT("Both pickups collected"), Other->IsComplete());
            Key(EKeys::Escape);
            break;
        case 11:
            Test->TestFalse(TEXT("Final modal closed"), Session->IsOpen());
            Test->TestFalse(TEXT("Input restored at end"), PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Gameplay input allowed again"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            Second->Destroy();
            return true;
        }
        Step++; Next = Now + (Step == 7 ? 1.3 : .4); return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    bool bWatchCompletedPrompt = false;
    FVector NearPosition = FVector::ZeroVector;
    TWeakObjectPtr<USearchableComponent> Source, Other;
    TWeakObjectPtr<AActor> Wardrobe, Second;
    TWeakObjectPtr<UActorComponent> Interaction;
};

class FTableInspectionFlow : public IAutomationLatentCommand
{
public:
    explicit FTableInspectionFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 60) { Test->AddError(TEXT("Table inspection timed out")); return true; }
        if (bWatch && Table.IsValid())
        {
            for (FName Name : {FName(TEXT("Widget_InteractionPrompt")), FName(TEXT("Ring_Visual"))})
            {
                auto* Indicator = FindSceneComponent(Table.Get(), Name);
                if (!Test->TestTrue(TEXT("Completed table indicators stay hidden across overlaps"), Indicator && Indicator->bHiddenInGame)) return true;
            }
        }
        if (Now < Next) return false;
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (!World) { if (Step) { Test->AddError(TEXT("PIE stopped during table inspection")); return true; } return false; }
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetPawn() || !PC->GetLocalPlayer()) return false;
        auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (Step == 0)
        {
            for (TActorIterator<AActor> It(World); It; ++It)
                if (auto* C = It->FindComponentByClass<USearchableComponent>())
                    if (C->Scene && C->Scene->GetFName() == TEXT("DA_Inspection_Table")) { Source = C; Table = *It; break; }
            if (!Test->TestNotNull(TEXT("Existing table has inspection configured"), Source.Get())) return true;
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction = C;
            auto* Point = FindSceneComponent(Table.Get(), TEXT("InteractionPoint"));
            if (!Test->TestNotNull(TEXT("Interaction component"), Interaction.Get()) || !Test->TestNotNull(TEXT("Table interaction point"), Point)) return true;
            auto* Data = Source->Scene.Get();
            FString Error;
            if (!Test->TestTrue(TEXT("Table scene data validates"), Data->ValidateScene(Error))) { Test->AddError(Error); return true; }
            if (!Test->TestEqual(TEXT("One note in table scene"), Data->Pickups.Num(), 1)) return true;
            Test->TestEqual(TEXT("Table background canvas preserves source dimensions"), Data->CanvasSize, FVector2D(2001,1561));
            Test->TestEqual(TEXT("Table uses supplied inspection background"), Data->Background->GetFName(), FName(TEXT("T_Inspection_Table")));
            Test->TestEqual(TEXT("Pickup placement ID"), Data->Pickups[0].PickupID, FName(TEXT("Table_Note")));
            Test->TestEqual(TEXT("Collected item type is Note"), Data->Pickups[0].ItemID, FName(TEXT("Note")));
            Test->TestEqual(TEXT("Pickup uses supplied note texture"), Data->Pickups[0].Image->GetFName(), FName(TEXT("T_Note")));
            Near = Point->GetComponentLocation(); Near.Z = PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Step++; Next = Now + .5; return false;
        }
        auto* C = Source.Get();
        if (!C) { Test->AddError(TEXT("Table source lost")); return true; }
        switch (Step)
        {
        case 1:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestTrue(TEXT("E interaction route opens table inspection"), Session->IsOpen())) return true;
            Test->TestEqual(TEXT("Inspection belongs to table"), Session->GetActiveSource(), C);
            Test->TestTrue(TEXT("Table inspection shows cursor and blocks movement"), PC->bShowMouseCursor && PC->IsMoveInputIgnored());
            Test->TestTrue(TEXT("Player stopped"), PC->GetPawn()->GetVelocity().IsNearlyZero());
            Test->TestFalse(TEXT("Gameplay input blocked during inspection"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            break;
        case 2:
        {
            auto* Widget = Session->GetActiveWidget();
            if (!Test->TestEqual(TEXT("Uncollected note displayed"), Widget->GetPickupButtons().Num(), 1)) return true;
            auto* Background = Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("Image_Background")));
            Test->TestTrue(TEXT("Shared widget displays table background"), Background && Background->GetBrush().GetResourceObject() == C->Scene->Background);
            const FVector2D DisplaySize = Widget->GetPickupButtons()[0]->GetCachedGeometry().GetLocalSize();
            Test->TestTrue(TEXT("Note rendered with original proportions"), DisplaySize.Y > 0 && FMath::IsNearlyEqual(DisplaySize.X / DisplaySize.Y, 656.0 / 769.0, .001));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("TableInspection/table-open.png"), true, false);
            break;
        }
        case 3:
            Click(Session->GetActiveWidget(), FVector2D(.05,.05)); Key(EKeys::Escape);
            Test->TestFalse(TEXT("Escape after background click closes table"), Session->IsOpen());
            Test->TestFalse(TEXT("Leaving note uncollected keeps table incomplete"), C->IsComplete());
            Test->TestTrue(TEXT("Unfinished table remains available"), UInspectionBlueprintLibrary::IsInteractionAvailable(Table.Get()));
            Test->TestFalse(TEXT("Abort restores cursor and movement"), PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            break;
        case 4:
            Invoke(Interaction.Get(), TEXT("TryInteract"));
            if (!Test->TestTrue(TEXT("Table reopens before collecting note"), Session->IsOpen())) return true;
            Test->TestEqual(TEXT("Note still present on re-entry"), Session->GetActiveWidget()->GetPickupButtons().Num(), 1);
            break;
        case 5:
            Test->TestTrue(TEXT("Actual mouse click on note"), Click(Session->GetActiveWidget()->GetPickupButtons()[0]));
            if (!Test->TestTrue(TEXT("Note click completes table"), C->IsComplete())) return true;
            Test->TestTrue(TEXT("Note collection recorded"), C->IsCollected(TEXT("Table_Note")));
            Test->TestEqual(TEXT("Note hidden after collection"), Session->GetActiveWidget()->GetPickupButtons()[0]->GetVisibility(), ESlateVisibility::Collapsed);
            Test->TestFalse(TEXT("Duplicate note collection rejected"), C->TryCollect(TEXT("Table_Note")));
            Test->TestTrue(TEXT("Last pickup keeps inspection open"), Session->IsOpen());
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("TableInspection/table-collected.png"), true, false);
            break;
        case 6:
            Key(EKeys::Escape);
            Test->TestFalse(TEXT("Escape after note click closes table"), Session->IsOpen());
            Test->TestFalse(TEXT("Completed table is unavailable"), UInspectionBlueprintLibrary::IsInteractionAvailable(Table.Get()));
            Test->TestFalse(TEXT("Completed table cannot open again"), Session->Open(C));
            Test->TestFalse(TEXT("Controls restored after collection"), PC->bShowMouseCursor || PC->IsMoveInputIgnored());
            bWatch = true;
            PC->GetPawn()->SetActorLocation(Near + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            break;
        case 7:
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Invoke(Interaction.Get(), TEXT("UpdateInteractionTarget"));
            if (auto* P = FindFProperty<FObjectProperty>(Interaction->GetClass(), TEXT("CurrentInteractable")))
                Test->TestNotEqual(TEXT("Completed table excluded from selection"), P->GetObjectPropertyValue_InContainer(Interaction.Get()), static_cast<UObject*>(Table.Get()));
            { struct { bool Visible = true; } Args; Invoke(Table.Get(), TEXT("SetAwarenessVisible"), &Args); }
            OtherActor = World->SpawnActor<AActor>(Table->GetClass(), FVector(5000,5000,0), FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Independent second table spawned"), OtherActor.Get())) return true;
            Other = OtherActor->FindComponentByClass<USearchableComponent>();
            if (!Test->TestNotNull(TEXT("Second table source"), Other.Get())) return true;
            Test->TestFalse(TEXT("Second table has independent progress"), Other->IsComplete());
            if (!Test->TestTrue(TEXT("Second table can open"), Session->Open(Other.Get()))) return true;
            break;
        case 8:
            Test->TestEqual(TEXT("Second table still has its own note"), Session->GetActiveWidget()->GetPickupButtons().Num(), 1);
            Key(EKeys::Escape);
            PC->GetPawn()->SetActorLocation(Near + FVector(600,0,0), false, nullptr, ETeleportType::TeleportPhysics);
            break;
        case 9:
            PC->GetPawn()->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
            Test->TestFalse(TEXT("Table furniture itself remains visible"), Table->IsHidden());
            Test->TestTrue(TEXT("Gameplay input restored"), UInspectionBlueprintLibrary::IsInteractionInputAllowed(PC->GetPawn()));
            OtherActor->Destroy();
            return true;
        }
        Step++; Next = Now + .35; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0;
    bool bWatch = false;
    FVector Near;
    TWeakObjectPtr<AActor> Table, OtherActor;
    TWeakObjectPtr<USearchableComponent> Source, Other;
    TWeakObjectPtr<UActorComponent> Interaction;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectionPIETest, "NENS.Inspection.Integration.PIEWardrobe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInspectionPIETest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FInspectionFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTableInspectionPIETest, "NENS.Inspection.Integration.PIETable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTableInspectionPIETest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FTableInspectionFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
