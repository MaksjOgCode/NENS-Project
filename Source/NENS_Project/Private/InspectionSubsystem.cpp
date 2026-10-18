#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "SearchableComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

bool UInspectionSubsystem::Open(USearchableComponent* Source)
{
    if (IsOpen() || !IsValid(Source) || Source->IsComplete() || !Source->Scene) return false;
    FString Error;
    if (!Source->Scene->ValidateScene(Error))
    {
        UE_LOG(LogTemp, Error, TEXT("Inspection data invalid: %s"), *Error);
        return false;
    }
    APlayerController* PC = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!PC || !PC->IsLocalController() || Source->GetWorld() != PC->GetWorld()) return false;
    UClass* WidgetClass = Source->SceneWidgetClass;
    if (!WidgetClass) return false;
    auto* Widget = CreateWidget<UInspectionSceneWidget>(PC, WidgetClass);
    if (!Widget || !Widget->Configure(Source, this)) return false;
    if (!Widget->AddToPlayerScreen(100)) return false;

    ActiveSource = Source;
    SessionController = PC;
    ActiveWidget = Widget;
    bPreviousCursor = PC->bShowMouseCursor;
    Source->GetOwner()->OnEndPlay.AddDynamic(this, &ThisClass::SourceEnded);
    if (APawn* Pawn = PC->GetPawn())
    {
        Pawn->ConsumeMovementInputVector();
        if (auto* Movement = Pawn->GetMovementComponent()) Movement->StopMovementImmediately();
    }
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, Widget, EMouseLockMode::DoNotLock, true);
    Widget->SetUserFocus(PC);
    return true;
}
void UInspectionSubsystem::Close()
{
    if (!ActiveWidget) return;
    USearchableComponent* Source = ActiveSource.Get();
    if (Source && Source->GetOwner()) Source->GetOwner()->OnEndPlay.RemoveDynamic(this, &ThisClass::SourceEnded);
    ActiveWidget->RemoveFromParent();
    ActiveWidget = nullptr;
    ActiveSource.Reset();
    if (APlayerController* PC = SessionController.Get())
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor = bPreviousCursor;
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, true);
    }
    SessionController.Reset();
    if (IsValid(Source)) Source->NotifyInspectionClosed();
}
void UInspectionSubsystem::SourceEnded(AActor*, EEndPlayReason::Type) { Close(); }
void UInspectionSubsystem::Deinitialize() { Close(); Super::Deinitialize(); }
