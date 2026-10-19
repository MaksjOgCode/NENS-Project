#include "EndGameWidget.h"
#include "GameExperienceSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InputCoreTypes.h"

namespace GamePresentation
{
UGameExperienceSubsystem* Flow(const UUserWidget* UI)
{
    auto* GI=UI->GetGameInstance(); return GI ? GI->GetSubsystem<UGameExperienceSubsystem>() : nullptr;
}
}
UEndGameWidget::UEndGameWidget(const FObjectInitializer& Init):Super(Init)
{
    SceneFadeInSeconds=.8f; SceneFadeOutSeconds=.6f; MistOpacity=.32f; MistHeight=.38f;
}
void UEndGameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_MainMenu")))) B->OnClicked.AddDynamic(this,&ThisClass::MainMenu);
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Quit")))) B->OnClicked.AddDynamic(this,&ThisClass::ExitGame);
}
void UEndGameWidget::MainMenu() { if (IsInteractive()) if (auto* F=GamePresentation::Flow(this)) F->ReturnToMainMenu(); }
void UEndGameWidget::ExitGame() { if (IsInteractive()) if (auto* F=GamePresentation::Flow(this)) F->QuitGame(); }
FReply UEndGameWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Escape) return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(G,Event);
}
void UMainMenuPresentationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Play")))) B->OnClicked.AddDynamic(this,&ThisClass::Play);
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Quit")))) B->OnClicked.AddDynamic(this,&ThisClass::ExitGame);
}
void UMainMenuPresentationWidget::NativeConstruct() { Super::NativeConstruct(); Age=0; SetRenderOpacity(0); }
void UMainMenuPresentationWidget::NativeTick(const FGeometry& G,float Dt)
{
    Super::NativeTick(G,Dt); Age+=Dt; const float T=FMath::Clamp(Age/.6f,0.f,1.f); SetRenderOpacity(T*T*(3-2*T));
}
void UMainMenuPresentationWidget::Play() { if (Age>=.6f) if (auto* F=GamePresentation::Flow(this)) F->StartNewGame(); }
void UMainMenuPresentationWidget::ExitGame() { if (Age>=.6f) if (auto* F=GamePresentation::Flow(this)) F->QuitGame(); }
FReply UMainMenuPresentationWidget::NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& Event)
{
    auto* F=GamePresentation::Flow(this);
    return Age<.6f || (F && F->IsTransitioning()) ? FReply::Handled() : Super::NativeOnPreviewMouseButtonDown(G,Event);
}
void UGameScreenFadeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetIsFocusable(true);
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this);
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Root;
    auto* Black=WidgetTree->ConstructWidget<UImage>();
    FSlateBrush Brush; Brush.DrawAs=ESlateBrushDrawType::Image; Brush.TintColor=FSlateColor(FLinearColor::Black); Black->SetBrush(Brush);
    auto* CanvasSlot=Root->AddChildToCanvas(Black); CanvasSlot->SetAnchors(FAnchors(0,0,1,1)); CanvasSlot->SetOffsets(FMargin(0));
}
