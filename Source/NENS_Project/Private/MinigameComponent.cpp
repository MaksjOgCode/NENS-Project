#include "MinigameComponent.h"
#include "MinigameWidget.h"
#include "InteractionSessionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UMinigameComponent::UMinigameComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UMinigameComponent::OpenMinigame(AActor* Interactor)
{
    if (bCompleted || ActiveWidget || !WidgetClass || !IsValid(Interactor)) return false;
    auto* PC = Cast<APlayerController>(Interactor);
    if (auto* Pawn = Cast<APawn>(Interactor)) PC = Cast<APlayerController>(Pawn->GetController());
    if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer()) return false;
    auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
    if (!Session || Session->IsOpen()) return false;
    auto* Widget = CreateWidget<UMinigameWidget>(PC, WidgetClass);
    if (!Widget) return false;
    Widget->InitializeSession(this);
    ActiveWidget = Widget;
    ActiveSession = Session;
    if (!Session->Open(Widget, GetOwner(), FSimpleDelegate::CreateUObject(this, &ThisClass::SessionClosed)))
    {
        ActiveWidget = nullptr; ActiveSession.Reset(); return false;
    }
    return true;
}
bool UMinigameComponent::ConfirmSuccess(UMinigameWidget* Reporter)
{
    auto* Session = ActiveSession.Get();
    if (bCompleted || !Reporter || ActiveWidget != Reporter || !Session || !Session->IsActive(Reporter)) return false;
    bCompleted = true;
    OnSucceeded.Broadcast();
    return true;
}
void UMinigameComponent::CloseMinigame(UMinigameWidget* Requester)
{
    if (Requester && ActiveWidget == Requester)
        if (auto* Session = ActiveSession.Get()) Session->Close(Requester);
}
void UMinigameComponent::SessionClosed()
{
    ActiveWidget = nullptr;
    ActiveSession.Reset();
    OnMinigameClosed.Broadcast(bCompleted);
}
