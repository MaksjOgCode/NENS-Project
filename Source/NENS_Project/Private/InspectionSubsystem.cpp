#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "SearchableComponent.h"
#include "InteractionSessionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

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
    auto* Session = GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
    if (!Session || Session->IsOpen()) return false;
    UClass* WidgetClass = Source->SceneWidgetClass;
    if (!WidgetClass) return false;
    auto* Widget = CreateWidget<UInspectionSceneWidget>(PC, WidgetClass);
    if (!Widget || !Widget->Configure(Source, this)) return false;
    ActiveSource = Source;
    ActiveWidget = Widget;
    ActiveSession = Session;
    if (!Session->Open(Widget, Source->GetOwner(), FSimpleDelegate::CreateUObject(this, &ThisClass::SessionClosed)))
    {
        ActiveWidget = nullptr; ActiveSource.Reset(); ActiveSession.Reset(); return false;
    }
    return true;
}
void UInspectionSubsystem::Close()
{
    if (ActiveWidget) ActiveWidget->BeginClose();
}
void UInspectionSubsystem::FinishClose(UInspectionSceneWidget* ExpectedWidget)
{
    if (!ExpectedWidget || ActiveWidget != ExpectedWidget) return;
    if (auto* Session = ActiveSession.Get()) Session->Close(ExpectedWidget);
    else { SessionClosed(); ExpectedWidget->RemoveFromParent(); }
}
void UInspectionSubsystem::SessionClosed()
{
    USearchableComponent* Source = ActiveSource.Get();
    ActiveWidget = nullptr;
    ActiveSource.Reset();
    ActiveSession.Reset();
    if (IsValid(Source)) Source->NotifyInspectionClosed();
}
void UInspectionSubsystem::Deinitialize() { FinishClose(ActiveWidget); Super::Deinitialize(); }
