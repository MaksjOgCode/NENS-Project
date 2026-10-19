#include "WiresMinigameWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/NativeWidgetHost.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "InspectionBackdrop.h"
#include "InspectionMist.h"

UWiresMinigameWidget::UWiresMinigameWidget(const FObjectInitializer& Init) : Super(Init)
{
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    InterfaceFont = Font.Object;
}

void UWiresMinigameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    auto* Frame = WidgetTree->FindWidget(TEXT("ScaleBox_Wires"));
    // Older/unconfigured assets keep their original game instead of losing their content.
    if (!Root || !Frame) return;
    if (auto* FrameSlot = Cast<UCanvasPanelSlot>(Frame->Slot))
        FrameSlot->SetOffsets(FMargin(FMath::Clamp(ScreenPadding, 0.f, 120.f)));

    auto* Backdrop = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), TEXT("WiresGradient"));
    Backdrop->SetContent(SNew(SInspectionBackdrop).SceneAspect(1.44f));
    Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BackdropSlot = Root->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0,0,1,1)); BackdropSlot->SetOffsets(FMargin(0)); BackdropSlot->SetZOrder(-10);

    auto* MistHost = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), TEXT("WiresMist"));
    Mist = SNew(SInspectionMist).Strength(MistOpacity);
    MistHost->SetContent(Mist.ToSharedRef());
    MistHost->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* MistSlot = Root->AddChildToCanvas(MistHost);
    MistSlot->SetAnchors(FAnchors(0,0,1,FMath::Clamp(MistHeight, .05f, .5f)));
    MistSlot->SetOffsets(FMargin(0)); MistSlot->SetZOrder(10);

    if (InterfaceFont)
        for (FName Name : {TEXT("TxtTitle"), TEXT("TxtStatus"), TEXT("Text_WiresControls"), TEXT("TextBlock_137")})
            if (auto* Text = Cast<UTextBlock>(WidgetTree->FindWidget(Name)))
            {
                FSlateFontInfo Font = Text->GetFont();
                Font.FontObject = InterfaceFont;
                Text->SetFont(Font); // Keep text sizes editable in the Widget Designer.
            }
}

void UWiresMinigameWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    MistTime += DeltaTime * FMath::Clamp(MistSpeed, 0.f, 2.f);
    if (Mist) Mist->SetTime(MistTime);
}

void UWiresMinigameWidget::NativeDestruct()
{
    Mist.Reset();
    Super::NativeDestruct();
}
