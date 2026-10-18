#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"
#include "SearchableComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

UInspectionPickupButton::UInspectionPickupButton(const FObjectInitializer& Init) : Super(Init)
{
    InitIsFocusable(false);
}
void UInspectionPickupButton::Configure(UInspectionSceneWidget* Scene, FName ID)
{
    SceneWidget = Scene;
    PickupID = ID;
    OnClicked.AddUniqueDynamic(this, &ThisClass::Collect);
}
void UInspectionPickupButton::Collect()
{
    if (auto* Scene = SceneWidget.Get()) Scene->CollectPickup(PickupID);
}
UInspectionSceneWidget::UInspectionSceneWidget(const FObjectInitializer& Init) : Super(Init)
{
    SetIsFocusable(true);
}
bool UInspectionSceneWidget::Configure(USearchableComponent* Source, UInspectionSubsystem* Session)
{
    SetIsFocusable(true);
    auto* Canvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("Canvas_Scene")));
    auto* Background = Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Background")));
    auto* Size = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("SizeBox_Scene")));
    if (!Canvas || !Background || !Size || !Source || !Source->Scene) return false;
    SourceComponent = Source;
    InspectionSession = Session;
    const UInspectionSceneData* Data = Source->Scene;
    Size->SetWidthOverride(Data->CanvasSize.X);
    Size->SetHeightOverride(Data->CanvasSize.Y);
    Background->SetBrushFromTexture(Data->Background);
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetVisibility(ESlateVisibility::Visible);

    for (const FInspectionPickup& Pickup : Data->Pickups)
    {
        if (Source->IsCollected(Pickup.PickupID)) continue;
        auto* Button = WidgetTree->ConstructWidget<UInspectionPickupButton>(UInspectionPickupButton::StaticClass());
        Button->Configure(this, Pickup.PickupID);
        FButtonStyle Style;
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.SetResourceObject(Pickup.Image);
        Brush.ImageSize = FVector2f(Pickup.Size);
        Style.SetNormal(Brush).SetHovered(Brush).SetPressed(Brush);
        Style.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
        Button->SetStyle(Style);
        auto* CanvasSlot = Canvas->AddChildToCanvas(Button);
        CanvasSlot->SetAnchors(FAnchors(0, 0));
        CanvasSlot->SetPosition(Pickup.Position);
        CanvasSlot->SetSize(Pickup.Size);
        CanvasSlot->SetZOrder(1);
        PickupButtons.Add(Button);
    }
    return true;
}
bool UInspectionSceneWidget::CollectPickup(FName PickupID)
{
    auto* Source = SourceComponent.Get();
    auto* Session = InspectionSession.Get();
    if (!Source || !Session || Session->GetActiveWidget() != this || !Source->TryCollect(PickupID)) return false;
    for (UInspectionPickupButton* Button : PickupButtons)
        if (Button->GetPickupID() == PickupID) Button->SetVisibility(ESlateVisibility::Collapsed);
    return true;
}
FReply UInspectionSceneWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape)
    {
        if (auto* Session = InspectionSession.Get()) Session->Close();
        return FReply::Handled();
    }
    // UI-only input mode prevents unhandled keys reaching gameplay.
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
FReply UInspectionSceneWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&)
{
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}
