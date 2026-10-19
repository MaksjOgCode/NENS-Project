#include "ClockInspectionWidget.h"
#include "ClockInspectionComponent.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "ClockInspection"

namespace ClockUI
{
FSlateFontInfo Font(UFont* Typeface, float Size)
{
    return Typeface ? FSlateFontInfo(Typeface, Size, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Regular", Size);
}
FVector2D Direction(float Degrees)
{
    const float Radians = FMath::DegreesToRadians(Degrees);
    return FVector2D(FMath::Sin(Radians), -FMath::Cos(Radians));
}
float Angle(FVector2D Delta) { return FRotator::ClampAxis(FMath::RadiansToDegrees(FMath::Atan2(Delta.X, -Delta.Y))); }
float DistanceToSegment(FVector2D P, FVector2D A, FVector2D B)
{
    const FVector2D AB = B - A;
    const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.0), 0.0, 1.0);
    return FVector2D::Distance(P, A + AB * T);
}
}

bool UClockInspectionWidget::Configure(USearchableComponent* Source, UInspectionSubsystem* InSession)
{
    auto* Component = Cast<UClockInspectionComponent>(Source);
    FString Error;
    if (!Component || !Component->ValidateSetup(Error)) { UE_LOG(LogTemp, Error, TEXT("Clock inspection: %s"), *Error); return false; }
    if (!Super::Configure(Source, InSession)) return false;
    Clock = Component; Session = InSession;
    auto* Data = Component->Puzzle.Get();
    SceneCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("Canvas_Scene")));
    OpenImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Background")));
    auto* Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("Canvas_Root")));
    if (!Root || !SceneCanvas || !OpenImage) return false;
    // The clock has its own readable help panel, including Escape, on both stages.
    if (auto* ExitHint = WidgetTree->FindWidget(TEXT("Text_ExitHint"))) ExitHint->SetVisibility(ESlateVisibility::Collapsed);

    ClosedImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ClockClosedBackground"));
    ClosedImage->SetBrushFromTexture(Data->ClosedBackground);
    ClosedImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BackgroundSlot = SceneCanvas->AddChildToCanvas(ClosedImage);
    BackgroundSlot->SetAnchors(FAnchors(0,0,1,1)); BackgroundSlot->SetOffsets(FMargin(0)); BackgroundSlot->SetZOrder(2);

    for (EClockHand Which : {EClockHand::Short, EClockHand::Long})
    {
        const auto& Art = Data->Hand(Which);
        auto* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Which == EClockHand::Short ? TEXT("ClockHourTray") : TEXT("ClockMinuteTray"));
        Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.44f,.43f,.40f,1), 8.f, FLinearColor(.1f,.09f,.08f,1), 2.f));
        Panel->SetBrushColor(FLinearColor::White);
        Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* PanelSlot = SceneCanvas->AddChildToCanvas(Panel);
        PanelSlot->SetPosition(Art.TrayPosition + FVector2D(-160,-340)); PanelSlot->SetSize(FVector2D(320,440)); PanelSlot->SetZOrder(3);
        TrayPanels.Add(Panel);
        auto* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Which == EClockHand::Short ? TEXT("ClockShortHand") : TEXT("ClockLongHand"));
        FSlateBrush Brush; Brush.SetResourceObject(Art.Texture); Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.ImageSize = FVector2f(Art.Size); Brush.SetUVRegion(FBox2f(FVector2f(Art.UVMin), FVector2f(Art.UVMax)));
        Image->SetBrush(Brush); Image->SetVisibility(ESlateVisibility::HitTestInvisible); Image->SetRenderTransformPivot(Art.Pivot);
        auto* HandSlot = SceneCanvas->AddChildToCanvas(Image); HandSlot->SetAlignment(Art.Pivot); HandSlot->SetZOrder(4);
        auto* Button = WidgetTree->ConstructWidget<UButton>();
        FButtonStyle ButtonStyle;
        ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White, 4.f));
        ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.86f,.84f,.78f,1), 4.f));
        ButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.65f,.63f,.58f,1), 4.f));
        ButtonStyle.SetNormalPadding(FMargin(8,4)).SetPressedPadding(FMargin(8,4));
        Button->SetStyle(ButtonStyle);
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetFont(ClockUI::Font(Data->InterfaceFont, 36));
        Label->SetText(Which == EClockHand::Short ? LOCTEXT("HourHand", "Hour hand") : LOCTEXT("MinuteHand", "Minute hand"));
        Label->SetJustification(ETextJustify::Center);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(.94f,.92f,.86f,1))); Button->AddChild(Label);
        auto* ButtonSlot = SceneCanvas->AddChildToCanvas(Button);
        ButtonSlot->SetPosition(Art.TrayPosition + FVector2D(-145,25)); ButtonSlot->SetSize(FVector2D(290,62)); ButtonSlot->SetZOrder(5);
        if (Which == EClockHand::Short) { ShortImage = Image; ShortButton = Button; Button->OnClicked.AddDynamic(this, &ThisClass::SelectShort); }
        else { LongImage = Image; LongButton = Button; Button->OnClicked.AddDynamic(this, &ThisClass::SelectLong); }
    }
    InstructionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ClockHelpPanel"));
    InstructionPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.025f,.025f,.025f,.94f), 5.f, FLinearColor(.32f,.31f,.28f,1), 1.f));
    InstructionPanel->SetBrushColor(FLinearColor::White);
    InstructionPanel->SetPadding(FMargin(22,12));
    InstructionPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    Instructions = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClockInstructions"));
    Instructions->SetFont(ClockUI::Font(Data->InterfaceFont, 22));
    Instructions->SetLineHeightPercentage(1.12f);
    Instructions->SetColorAndOpacity(FSlateColor(FLinearColor(.94f,.92f,.86f,1)));
    Instructions->SetJustification(ETextJustify::Left); Instructions->SetVisibility(ESlateVisibility::HitTestInvisible);
    InstructionPanel->AddChild(Instructions);
    auto* HintSlot = Root->AddChildToCanvas(InstructionPanel);
    HintSlot->SetAnchors(FAnchors(.5,1)); HintSlot->SetAlignment(FVector2D(.5,1)); HintSlot->SetPosition(FVector2D(0,-20)); HintSlot->SetAutoSize(true); HintSlot->SetZOrder(10);
    if (Component->IsPuzzleSolved()) ShowCompartment();
    else
    {
        OpenImage->SetRenderOpacity(0);
        for (UInspectionPickupButton* Button : GetPickupButtons()) Button->SetVisibility(ESlateVisibility::Collapsed);
        RefreshHands();
    }
    return true;
}
bool UClockInspectionWidget::IsCurrent() const { return Clock.IsValid() && Session.IsValid() && Session->GetActiveWidget() == this; }
bool UClockInspectionWidget::IsPuzzleInteractive() const { return IsInspectionInteractive() && IsCurrent() && !Clock->IsPuzzleSolved() && !bTransitioning; }
float UClockInspectionWidget::DisplayAngle(EClockHand Hand) const
{
    if (Drag == EDrag::Rotate && Hand == Selected) return PreviewAngle;
    // Present both loose hands upright, so their lengths can be compared directly.
    return Clock->IsHandPlaced(Hand) ? Clock->GetHandAngle(Hand) : 0.f;
}
FVector2D UClockInspectionWidget::HandAnchor(EClockHand Hand) const
{
    if (Drag == EDrag::FromTray && Hand == Selected) return CursorScene - GrabOffset;
    return Clock->IsHandPlaced(Hand) ? Clock->Puzzle->DialCenter : Clock->Puzzle->Hand(Hand).TrayPosition;
}
FVector2D UClockInspectionWidget::HandTip(EClockHand Hand) const
{
    const float Scale = Clock->IsHandPlaced(Hand) ? 1.f : Clock->Puzzle->TrayScale;
    return HandAnchor(Hand) + ClockUI::Direction(DisplayAngle(Hand)) * Clock->Puzzle->Hand(Hand).TipLength * Scale;
}
void UClockInspectionWidget::RefreshHands()
{
    if (!Clock.IsValid() || !ShortImage || bCompartmentVisible) return;
    for (EClockHand Hand : {EClockHand::Short, EClockHand::Long})
    {
        const auto& Art = Clock->Puzzle->Hand(Hand);
        auto* Image = Hand == EClockHand::Short ? ShortImage.Get() : LongImage.Get();
        auto* HandSlot = CastChecked<UCanvasPanelSlot>(Image->Slot);
        HandSlot->SetPosition(HandAnchor(Hand)); HandSlot->SetSize(Art.Size * (Clock->IsHandPlaced(Hand) ? 1.0 : Clock->Puzzle->TrayScale));
        Image->SetRenderTransformAngle(DisplayAngle(Hand) - Art.NativeAngle);
        Image->SetRenderOpacity(Hand == Selected ? 1 : .8);
    }
    for (int32 Index = 0; Index < TrayPanels.Num(); ++Index)
        TrayPanels[Index]->SetRenderOpacity(Clock->IsHandPlaced(Index == 0 ? EClockHand::Short : EClockHand::Long) ? .45f : 1.f);
    ShortButton->SetBackgroundColor(Selected == EClockHand::Short ? FLinearColor(.28f,.27f,.24f,1) : FLinearColor(.12f,.115f,.10f,1));
    LongButton->SetBackgroundColor(Selected == EClockHand::Long ? FLinearColor(.28f,.27f,.24f,1) : FLinearColor(.12f,.115f,.10f,1));
    Instructions->SetText(LOCTEXT("Controls", "Drag both hands onto the clock face.\nDrag a hand's tip around the dial to turn it.\nMouse wheel or Left / Right: fine adjustment.\nTab: switch hands.    Esc: return to the room."));
    if (Drag == EDrag::FromTray)
        Instructions->SetText(FText::FromString(FVector2D::Distance(CursorScene, Clock->Puzzle->DialCenter) <= Clock->Puzzle->DialRadius + 30
            ? TEXT("Release the left mouse button to attach the hand.\nEsc: cancel and return to the room.")
            : TEXT("Hold the left mouse button and drag the hand onto the clock face.\nRelease outside the dial to put it back.    Esc: return to the room.")));
}
void UClockInspectionWidget::SelectHand(EClockHand Hand) { if (IsPuzzleInteractive() && Drag == EDrag::None) { Selected = Hand; RefreshHands(); } }
void UClockInspectionWidget::SelectShort() { SelectHand(EClockHand::Short); }
void UClockInspectionWidget::SelectLong() { SelectHand(EClockHand::Long); }

bool UClockInspectionWidget::HitHand(FVector2D Point, EClockHand Hand) const
{
    const FVector2D Anchor = HandAnchor(Hand);
    return ClockUI::DistanceToSegment(Point, Anchor, HandTip(Hand)) <= (Clock->IsHandPlaced(Hand) ? 28 : 48);
}
FReply UClockInspectionWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton && IsPuzzleInteractive())
    {
        CursorScene = SceneCanvas->GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition());
        const EClockHand Other = Selected == EClockHand::Short ? EClockHand::Long : EClockHand::Short;
        bool bHit = HitHand(CursorScene, Selected);
        if (!bHit && HitHand(CursorScene, Other)) { Selected = Other; bHit = true; }
        if (!bHit && Clock->IsHandPlaced(Selected)) bHit = FVector2D::Distance(CursorScene, Clock->Puzzle->DialCenter) <= Clock->Puzzle->DialRadius;
        if (bHit)
        {
            GrabOffset = CursorScene - HandAnchor(Selected);
            Drag = Clock->IsHandPlaced(Selected) ? EDrag::Rotate : EDrag::FromTray;
            PreviewAngle = Clock->GetHandAngle(Selected);
            RefreshHands();
            return FReply::Handled().SetUserFocus(TakeWidget()).CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}
FReply UClockInspectionWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Drag != EDrag::None && IsPuzzleInteractive())
    {
        CursorScene = SceneCanvas->GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition());
        const FVector2D Delta = CursorScene - Clock->Puzzle->DialCenter;
        if (Drag == EDrag::Rotate && Delta.Size() > 15) PreviewAngle = ClockUI::Angle(Delta);
        RefreshHands();
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(Geometry, Event);
}
FReply UClockInspectionWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Drag != EDrag::None)
    {
        const EDrag PreviousDrag = Drag;
        CursorScene = SceneCanvas->GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition());
        Drag = EDrag::None;
        if (IsPuzzleInteractive())
        {
            if (PreviousDrag == EDrag::FromTray)
            {
                if (FVector2D::Distance(CursorScene, Clock->Puzzle->DialCenter) <= Clock->Puzzle->DialRadius + 30) Clock->PlaceHand(Selected);
            }
            else Clock->RotateHand(Selected, FMath::GridSnap(PreviewAngle, Clock->Puzzle->SnapDegrees));
            if (Clock->IsPuzzleSolved()) BeginTransition(); else RefreshHands();
        }
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(Geometry, Event);
}
void UClockInspectionWidget::RotateSelected(float Delta)
{
    if (IsPuzzleInteractive() && Drag == EDrag::None && Clock->IsHandPlaced(Selected))
    {
        Clock->RotateHand(Selected, Clock->GetHandAngle(Selected) + Delta);
        if (Clock->IsPuzzleSolved()) BeginTransition(); else RefreshHands();
    }
}
FReply UClockInspectionWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (IsPuzzleInteractive()) { RotateSelected(Event.GetWheelDelta() * Clock->Puzzle->SnapDegrees); return FReply::Handled(); }
    return Super::NativeOnMouseWheel(Geometry, Event);
}
FReply UClockInspectionWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!IsCurrent()) return FReply::Handled();
    if (Event.GetKey() == EKeys::Escape)
    {
        Drag = EDrag::None; // Cancel the uncommitted gesture; keep the last stored angle.
        Super::NativeOnPreviewKeyDown(Geometry, Event);
        return FReply::Handled().ReleaseMouseCapture();
    }
    if (IsPuzzleInteractive() && Event.GetKey() == EKeys::Tab)
    { SelectHand(Selected == EClockHand::Short ? EClockHand::Long : EClockHand::Short); return FReply::Handled(); }
    if (IsPuzzleInteractive() && (Event.GetKey() == EKeys::Left || Event.GetKey() == EKeys::Right))
    { RotateSelected((Event.GetKey() == EKeys::Right ? 1 : -1) * Clock->Puzzle->SnapDegrees); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
void UClockInspectionWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    Drag = EDrag::None;
    if (IsPuzzleInteractive()) RefreshHands();
    Super::NativeOnMouseCaptureLost(Event);
}
void UClockInspectionWidget::BeginTransition()
{
    if (!IsCurrent() || bTransitioning || bCompartmentVisible) return;
    Drag = EDrag::None; bTransitioning = true; TransitionTime = 0;
    ShortButton->SetVisibility(ESlateVisibility::Collapsed); LongButton->SetVisibility(ESlateVisibility::Collapsed);
    Instructions->SetText(LOCTEXT("Opening", "The clock clicks open...\nEsc: return to the room."));
}
void UClockInspectionWidget::ShowCompartment()
{
    bCompartmentVisible = true; bTransitioning = false;
    ClosedImage->SetVisibility(ESlateVisibility::Collapsed);
    ShortImage->SetVisibility(ESlateVisibility::Collapsed); LongImage->SetVisibility(ESlateVisibility::Collapsed);
    ShortButton->SetVisibility(ESlateVisibility::Collapsed); LongButton->SetVisibility(ESlateVisibility::Collapsed);
    OpenImage->SetRenderOpacity(1);
    for (UWidget* Panel : TrayPanels) Panel->SetVisibility(ESlateVisibility::Collapsed);
    for (UInspectionPickupButton* Button : GetPickupButtons()) Button->SetVisibility(ESlateVisibility::Visible);
    Instructions->SetText(LOCTEXT("TakeCard", "Click the punch card to collect it.\nEsc: return to the room."));
}
void UClockInspectionWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (!IsCurrent()) return;
    const float Width = FMath::Clamp(static_cast<float>(Geometry.GetLocalSize().X) - 92.f, 200.f, 640.f);
    if (!FMath::IsNearlyEqual(Width, HintWidth)) { HintWidth = Width; Instructions->SetWrapTextAt(Width); }
    if (Clock->IsPuzzleSolved() && !bCompartmentVisible && !bTransitioning) BeginTransition();
    if (bTransitioning)
    {
        TransitionTime += DeltaTime;
        const float Alpha = FMath::Clamp(TransitionTime / Clock->Puzzle->TransitionSeconds, 0.f, 1.f);
        const float Blend = Alpha * Alpha * (3 - 2 * Alpha);
        OpenImage->SetRenderOpacity(Blend); ClosedImage->SetRenderOpacity(1 - Blend);
        ShortImage->SetRenderOpacity(1 - Blend); LongImage->SetRenderOpacity(1 - Blend);
        for (UWidget* Panel : TrayPanels) Panel->SetRenderOpacity(1 - Blend);
        if (Alpha >= 1) ShowCompartment();
    }
    if (bCompartmentVisible && Clock->IsComplete()) Instructions->SetText(LOCTEXT("Collected", "Punch card collected.\nEsc: return to the room."));
}

void UClockInspectionWidget::NativeDestruct()
{
    Drag = EDrag::None;
    if (HasMouseCapture() && FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseAllPointerCapture();
    Clock.Reset(); Session.Reset();
    Super::NativeDestruct();
}
void UClockInspectionWidget::OnCloseStarted()
{
    Drag = EDrag::None;
    if (Clock.IsValid() && !Clock->IsPuzzleSolved()) RefreshHands();
}

#undef LOCTEXT_NAMESPACE
