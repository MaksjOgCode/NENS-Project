#include "MinigameWidget.h"
#include "MinigameComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/NativeWidgetHost.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "InspectionBackdrop.h"
#include "InspectionMist.h"

UMinigameWidget::UMinigameWidget(const FObjectInitializer& Init) : Super(Init)
{
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    InterfaceFont = Font.Object;
}
void UMinigameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    SetRenderOpacity(SceneFadeInSeconds > 0 ? 0.f : 1.f);
    auto* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    const bool bWires = WidgetTree->FindWidget(TEXT("ScaleBox_Wires")) != nullptr;
    auto* Frame = WidgetTree->FindWidget(bWires ? TEXT("ScaleBox_Wires") : TEXT("ScaleBox_Minigame"));
    if (!Root || !Frame) return;
    if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(Frame->Slot)) CanvasSlot->SetOffsets(FMargin(FMath::Clamp(ScreenPadding, 0.f, 120.f)));
    auto* Backdrop = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), bWires ? TEXT("WiresGradient") : TEXT("MinigameGradient"));
    Backdrop->SetContent(SNew(SInspectionBackdrop).SceneAspect(bWires ? 1.44f : 1.64f));
    Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* CanvasSlot = Root->AddChildToCanvas(Backdrop);
    CanvasSlot->SetAnchors(FAnchors(0,0,1,1)); CanvasSlot->SetOffsets(FMargin(0)); CanvasSlot->SetZOrder(-10);
    auto* Host = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), bWires ? TEXT("WiresMist") : TEXT("MinigameMist"));
    Mist = SNew(SInspectionMist).Strength(MistOpacity);
    Host->SetContent(Mist.ToSharedRef()); Host->SetVisibility(ESlateVisibility::HitTestInvisible);
    CanvasSlot = Root->AddChildToCanvas(Host);
    CanvasSlot->SetAnchors(FAnchors(0,0,1,FMath::Clamp(MistHeight,.05f,.5f))); CanvasSlot->SetOffsets(FMargin(0)); CanvasSlot->SetZOrder(10);
    if (InterfaceFont)
        WidgetTree->ForEachWidget([this](UWidget* Widget)
        {
            if (auto* Text = Cast<UTextBlock>(Widget))
            {
                auto Font = Text->GetFont(); Font.FontObject = InterfaceFont; Text->SetFont(Font);
            }
        });
}
void UMinigameWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!bManagedSession)
    {
        auto* PC = GetOwningPlayer();
        if (!PC) { PC = UGameplayStatics::GetPlayerController(this,0); if (PC) SetOwningPlayer(PC); }
        if (PC)
        {
            bStandaloneInput = true;
            PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor = true;
            UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC,this,EMouseLockMode::DoNotLock,true);
            SetUserFocus(PC);
        }
    }
}
void UMinigameWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);
    if (bRemoved) return;
    MistTime += DeltaTime * FMath::Clamp(MistSpeed,0.f,2.f);
    if (Mist) Mist->SetTime(MistTime);
    FadeAge += DeltaTime;
    if (bClosing)
    {
        const float Duration = FMath::Max(0.f, SceneFadeOutSeconds) * CloseOpacity;
        const float T = Duration > 0 ? FMath::Clamp(FadeAge/Duration,0.f,1.f) : 1.f;
        SetRenderOpacity(CloseOpacity * (1.f-T*T*(3.f-2.f*T)));
        if (T >= 1) FinishClose();
    }
    else
    {
        const float T = SceneFadeInSeconds > 0 ? FMath::Clamp(FadeAge/SceneFadeInSeconds,0.f,1.f) : 1.f;
        SetRenderOpacity(T*T*(3.f-2.f*T));
    }
}
void UMinigameWidget::InitializeSession(UMinigameComponent* Source)
{
    SourceComponent = Source; bManagedSession = true;
}
void UMinigameWidget::ReportSuccess()
{
    if (bReportedSuccess || !IsInteractive() || !CanReportSuccess()) return;
    if (bManagedSession)
    {
        if (auto* Source = SourceComponent.Get()) bReportedSuccess = Source->ConfirmSuccess(this);
    }
    else bReportedSuccess = true;
}
void UMinigameWidget::RequestClose()
{
    if (bRemoved || bClosing) return;
    bClosing = true; FadeAge = 0; CloseOpacity = GetRenderOpacity();
    StopAnimationsAndLatentActions();
    if (HasMouseCapture() && FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseAllPointerCapture();
    // Keep ownership and gameplay input locked until the final transparent frame.
    if (SceneFadeOutSeconds <= 0 || CloseOpacity <= 0) FinishClose();
}
void UMinigameWidget::FinishClose()
{
    if (bRemoved) return;
    bRemoved = true;
    if (bManagedSession)
    {
        // A late callback from a retired attempt cannot affect a newer modal.
        if (auto* Source = SourceComponent.Get()) Source->CloseMinigame(this);
    }
    else { RestoreStandaloneInput(); RemoveFromParent(); }
}
void UMinigameWidget::RestoreStandaloneInput()
{
    if (!bStandaloneInput) return;
    bStandaloneInput = false;
    if (auto* PC = GetOwningPlayer())
    {
        PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false); PC->bShowMouseCursor = false;
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC,true);
    }
}
FReply UMinigameWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape) { RequestClose(); return FReply::Handled(); }
    return IsInteractive() ? Super::NativeOnPreviewKeyDown(Geometry,Event) : FReply::Handled();
}
FReply UMinigameWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    return IsInteractive() ? Super::NativeOnPreviewMouseButtonDown(Geometry,Event) : FReply::Handled();
}
FReply UMinigameWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&)
{
    return FReply::Handled().SetUserFocus(TakeWidget(),EFocusCause::Mouse);
}
void UMinigameWidget::NativeDestruct()
{
    bRemoved = true; Mist.Reset(); RestoreStandaloneInput();
    if (bManagedSession)
        if (auto* Source = SourceComponent.Get()) Source->CloseMinigame(this);
    Super::NativeDestruct();
}
