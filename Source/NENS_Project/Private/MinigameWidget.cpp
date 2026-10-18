#include "MinigameWidget.h"
#include "MinigameComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UMinigameWidget::NativeOnInitialized() { Super::NativeOnInitialized(); SetIsFocusable(true); }
void UMinigameWidget::InitializeSession(UMinigameComponent* Source)
{
    SourceComponent = Source;
    bManagedSession = true;
    SetIsFocusable(true);
}
void UMinigameWidget::ReportSuccess()
{
    if (bReportedSuccess) return;
    if (bManagedSession)
    {
        if (auto* Source = SourceComponent.Get()) bReportedSuccess = Source->ConfirmSuccess(this);
    }
    else bReportedSuccess = true; // The original test levels still work independently.
}
void UMinigameWidget::RequestClose()
{
    if (bManagedSession)
    {
        // A late callback from a retired attempt must never affect a newer modal.
        if (auto* Source = SourceComponent.Get()) Source->CloseMinigame(this);
        return;
    }
    StopAnimationsAndLatentActions();
    if (auto* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = false;
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, true);
    }
    RemoveFromParent();
}
FReply UMinigameWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape) { RequestClose(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
FReply UMinigameWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&)
{
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}
void UMinigameWidget::NativeDestruct()
{
    if (bManagedSession)
        if (auto* Source = SourceComponent.Get()) Source->CloseMinigame(this);
    Super::NativeDestruct();
}
