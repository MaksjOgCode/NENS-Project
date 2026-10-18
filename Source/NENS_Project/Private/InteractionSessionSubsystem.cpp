#include "InteractionSessionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

bool UInteractionSessionSubsystem::Open(UUserWidget* Widget, AActor* Owner, FSimpleDelegate Closed)
{
    if (IsOpen() || !IsValid(Widget) || !IsValid(Owner)) return false;
    auto* PC = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (!PC || !PC->IsLocalController() || Owner->GetWorld() != PC->GetWorld() || Widget->GetOwningPlayer() != PC) return false;
    Widget->SetIsFocusable(true);
    Widget->SetVisibility(ESlateVisibility::Visible);
    if (!Widget->AddToPlayerScreen(100)) return false;
    ActiveWidget = Widget;
    ActiveOwner = Owner;
    SessionController = PC;
    OnClosed = MoveTemp(Closed);
    bPreviousCursor = PC->bShowMouseCursor;
    Owner->OnEndPlay.AddDynamic(this, &ThisClass::OwnerEnded);
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
void UInteractionSessionSubsystem::Close(UUserWidget* ExpectedWidget)
{
    if (!IsActive(ExpectedWidget) || bClosing) return;
    bClosing = true;
    UUserWidget* ClosingWidget = ActiveWidget;
    FSimpleDelegate Callback = MoveTemp(OnClosed);
    if (auto* Owner = ActiveOwner.Get()) Owner->OnEndPlay.RemoveDynamic(this, &ThisClass::OwnerEnded);
    // Clear ownership before RemoveFromParent, which can trigger widget Destruct.
    ActiveWidget = nullptr;
    ActiveOwner.Reset();
    ClosingWidget->StopAnimationsAndLatentActions();
    ClosingWidget->RemoveFromParent();
    if (auto* PC = SessionController.Get())
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor = bPreviousCursor;
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, true);
    }
    SessionController.Reset();
    bClosing = false;
    Callback.ExecuteIfBound();
}
void UInteractionSessionSubsystem::OwnerEnded(AActor*, EEndPlayReason::Type) { Close(ActiveWidget); }
void UInteractionSessionSubsystem::Deinitialize() { Close(ActiveWidget); Super::Deinitialize(); }
