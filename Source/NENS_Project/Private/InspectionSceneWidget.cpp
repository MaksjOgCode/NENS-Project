#include "InspectionSceneWidget.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"
#include "SearchableComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/NativeWidgetHost.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "InspectionBackdrop.h"
#include "InspectionMist.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "InspectionScene"

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
void UInspectionPickupButton::BeginCollectionFade(float Seconds)
{
    FadeTime = 0;
    FadeDuration = FMath::IsFinite(Seconds) ? FMath::Clamp(Seconds, 0.f, 2.f) : .4f;
    bCollectionFading = true;
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (FadeDuration == 0) FinishCollectionFade();
}
void UInspectionPickupButton::AdvanceCollectionFade(float DeltaTime)
{
    if (!bCollectionFading) return;
    FadeTime += DeltaTime;
    const float Alpha = FMath::Clamp(FadeTime / FadeDuration, 0.f, 1.f);
    SetRenderOpacity(1 - Alpha * Alpha * (3 - 2 * Alpha));
    if (Alpha >= 1) FinishCollectionFade();
}
void UInspectionPickupButton::FinishCollectionFade()
{
    if (!bCollectionFading) return;
    bCollectionFading = false;
    SetRenderOpacity(0);
    SetVisibility(ESlateVisibility::Collapsed);
}
UInspectionSceneWidget::UInspectionSceneWidget(const FObjectInitializer& Init) : Super(Init)
{
    SetIsFocusable(true);
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    InterfaceFont = Font.Object;
}
bool UInspectionSceneWidget::Configure(USearchableComponent* Source, UInspectionSubsystem* Session)
{
    SetIsFocusable(true);
    auto* Canvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("Canvas_Scene")));
    auto* Background = Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Background")));
    auto* Size = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("SizeBox_Scene")));
    auto* Root = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("Canvas_Root")));
    if (!Root || !Canvas || !Background || !Size || !Source || !Source->Scene) return false;
    InspectionCanvas = Canvas;
    SourceComponent = Source;
    InspectionSession = Session;
    const UInspectionSceneData* Data = Source->Scene;
    bSceneHasPickups = !Data->Pickups.IsEmpty();
    PresentationTime = 0; bClosing = false;
    bOpening = FMath::IsFinite(SceneFadeInSeconds) && SceneFadeInSeconds > 0;
    SetRenderOpacity(bOpening ? 0.f : 1.f);
    Size->SetWidthOverride(Data->CanvasSize.X);
    Size->SetHeightOverride(Data->CanvasSize.Y);
    Background->SetBrushFromTexture(Data->Background);
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetVisibility(ESlateVisibility::Visible);

    if (auto* OldBackdrop = WidgetTree->FindWidget(TEXT("Image_Backdrop"))) OldBackdrop->SetVisibility(ESlateVisibility::Collapsed);
    if (auto* OldHint = WidgetTree->FindWidget(TEXT("Text_ExitHint"))) OldHint->SetVisibility(ESlateVisibility::Collapsed);
    auto* Backdrop = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), TEXT("InspectionGradient"));
    Backdrop->SetContent(SNew(SInspectionBackdrop).SceneAspect(Data->CanvasSize.X / Data->CanvasSize.Y));
    Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BackdropSlot = Root->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0,0,1,1)); BackdropSlot->SetOffsets(FMargin(0)); BackdropSlot->SetZOrder(0);

    auto* MistHost = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), TEXT("InspectionMist"));
    Mist = SNew(SInspectionMist).Strength(MistOpacity);
    MistHost->SetContent(Mist.ToSharedRef());
    MistHost->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* MistSlot = Root->AddChildToCanvas(MistHost);
    MistSlot->SetAnchors(FAnchors(0,0,1,FMath::Clamp(MistHeight, .05f, .5f)));
    MistSlot->SetOffsets(FMargin(0)); MistSlot->SetZOrder(3);

    if (UsesDefaultInspectionHints())
    {
        HelpPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InspectionHelpPanel"));
        HelpPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.015f,.015f,.015f,.85f), 5.f));
        HelpPanel->SetPadding(FMargin(16,14));
        HelpPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        HelpPanel->AddChild(Content);
        HelpTitle = WidgetTree->ConstructWidget<UTextBlock>();
        HelpTitle->SetFont(InterfaceFont ? FSlateFontInfo(InterfaceFont, 14, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Regular", 14));
        HelpTitle->SetText(LOCTEXT("ControlsTitle", "CONTROLS"));
        HelpTitle->SetColorAndOpacity(FSlateColor(FLinearColor(.55f,.54f,.5f,1)));
        auto* TitleSlot = Content->AddChildToVerticalBox(HelpTitle); TitleSlot->SetPadding(FMargin(0,0,0,16));
        HelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InspectionControls"));
        HelpText->SetFont(InterfaceFont ? FSlateFontInfo(InterfaceFont, 22, TEXT("Regular")) : FCoreStyle::GetDefaultFontStyle("Regular", 22));
        HelpText->SetLineHeightPercentage(1.12f);
        HelpText->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1)));
        HelpText->SetText(bSceneHasPickups ? LOCTEXT("PickupControls", "Esc — Exit\n\nLeft click — Pick up an item") : LOCTEXT("ArtControls", "Esc — Exit"));
        Content->AddChildToVerticalBox(HelpText);
        auto* HelpSlot = Root->AddChildToCanvas(HelpPanel); HelpSlot->SetAutoSize(true); HelpSlot->SetZOrder(5);
    }

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
    if (!IsInspectionInteractive() || !Source || !Session || !Source->TryCollect(PickupID)) return false;
    // Progress is committed before the visual fade. Subscribers may have closed the session.
    if (InspectionSession.IsValid() && InspectionSession->GetActiveWidget() == this)
        for (UInspectionPickupButton* Button : PickupButtons)
            if (Button->GetPickupID() == PickupID) Button->BeginCollectionFade(PickupFadeSeconds);
    return true;
}
bool UInspectionSceneWidget::IsInspectionInteractive() const
{
    return !bOpening && !bClosing && InspectionSession.IsValid() && InspectionSession->GetActiveWidget() == this;
}
void UInspectionSceneWidget::BeginClose()
{
    auto* Session = InspectionSession.Get();
    if (!Session || Session->GetActiveWidget() != this || bClosing) return;
    ClosingOpacity = GetRenderOpacity();
    ClosingDuration = FMath::IsFinite(SceneFadeOutSeconds) ? FMath::Clamp(SceneFadeOutSeconds, 0.f, 2.f) * ClosingOpacity : .24f * ClosingOpacity;
    bClosing = true; bOpening = false; PresentationTime = 0;
    OnCloseStarted();
    if (HasMouseCapture() && FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseAllPointerCapture();
    if (ClosingDuration <= KINDA_SMALL_NUMBER) Session->FinishClose(this);
}
void UInspectionSceneWidget::UpdateHelpLayout(const FGeometry& Geometry)
{
    if (!HelpPanel || !InspectionCanvas) return;
    const FVector2D Viewport = Geometry.GetLocalSize();
    const float Side = FMath::Max(0.f, static_cast<float>(Geometry.AbsoluteToLocal(InspectionCanvas->GetCachedGeometry().LocalToAbsolute(FVector2D::ZeroVector)).X));
    if (Viewport.Equals(LastHelpViewport, .5) && FMath::IsNearlyEqual(Side, LastSideMargin, .5f)) return;
    LastHelpViewport = Viewport; LastSideMargin = Side;
    auto* HelpSlot = CastChecked<UCanvasPanelSlot>(HelpPanel->Slot);
    const bool bSidebar = Side >= 190;
    HelpSlot->SetAnchors(FAnchors(0, bSidebar ? .5f : 1.f));
    HelpSlot->SetAlignment(FVector2D(0, bSidebar ? .5f : 1.f));
    HelpSlot->SetPosition(FVector2D(bSidebar ? 24 : 16, bSidebar ? 0 : -16));
    HelpPanel->SetBrushColor(FLinearColor(1,1,1,bSidebar ? .25f : 1.f));
    HelpTitle->SetVisibility(bSidebar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    HelpText->SetWrapTextAt(bSidebar ? Side - 80 : FMath::Clamp(static_cast<float>(Viewport.X) - 64, 120.f, 440.f));
    HelpText->SetText(bSceneHasPickups
        ? (bSidebar ? LOCTEXT("PickupControls", "Esc — Exit\n\nLeft click — Pick up an item") : LOCTEXT("CompactPickupControls", "Esc — Exit\nLeft click — Pick up an item"))
        : LOCTEXT("ArtControls", "Esc — Exit"));
}
void UInspectionSceneWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    auto* Session = InspectionSession.Get();
    if (!Session || Session->GetActiveWidget() != this) return;
    MistTime += DeltaTime * FMath::Clamp(MistSpeed, 0.f, 2.f);
    if (Mist) Mist->SetTime(MistTime);
    PresentationTime += DeltaTime;
    if (bClosing)
    {
        const float Alpha = FMath::Clamp(PresentationTime / FMath::Max(ClosingDuration, SMALL_NUMBER), 0.f, 1.f);
        SetRenderOpacity(ClosingOpacity * (1 - Alpha * Alpha * (3 - 2 * Alpha)));
        if (Alpha >= 1) { Session->FinishClose(this); return; }
    }
    else if (bOpening)
    {
        const float Alpha = FMath::Clamp(PresentationTime / FMath::Clamp(SceneFadeInSeconds, .001f, 2.f), 0.f, 1.f);
        SetRenderOpacity(Alpha * Alpha * (3 - 2 * Alpha));
        if (Alpha >= 1) bOpening = false;
    }
    UpdateHelpLayout(Geometry);
    for (UInspectionPickupButton* Button : PickupButtons) Button->AdvanceCollectionFade(DeltaTime);
}
void UInspectionSceneWidget::NativeDestruct()
{
    for (UInspectionPickupButton* Button : PickupButtons) Button->FinishCollectionFade();
    if (auto* Session = InspectionSession.Get()) Session->FinishClose(this);
    Mist.Reset();
    SourceComponent.Reset(); InspectionSession.Reset();
    Super::NativeDestruct();
}
FReply UInspectionSceneWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape)
    {
        if (auto* Session = InspectionSession.Get()) if (Session->GetActiveWidget() == this) Session->Close();
        return FReply::Handled();
    }
    // UI-only input mode prevents unhandled keys reaching gameplay.
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

#undef LOCTEXT_NAMESPACE
FReply UInspectionSceneWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&)
{
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}
