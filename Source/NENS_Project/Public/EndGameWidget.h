#pragma once
#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "EndGameWidget.generated.h"

UCLASS()
class NENS_PROJECT_API UEndGameWidget : public UMinigameWidget
{
    GENERATED_BODY()
public:
    UEndGameWidget(const FObjectInitializer& Init);
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event) override;
    virtual bool CanReportSuccess() const override { return false; }
private:
    UFUNCTION() void MainMenu();
    UFUNCTION() void ExitGame();
};

/** Adapter preserves the artists' main-menu layout and blink animation. */
UCLASS()
class NENS_PROJECT_API UMainMenuPresentationWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& G,float Dt) override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& Event) override;
private:
    float Age=0;
    UFUNCTION() void Play();
    UFUNCTION() void ExitGame();
};

UCLASS()
class NENS_PROJECT_API UGameScreenFadeWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override { return FReply::Handled(); }
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override { return FReply::Handled(); }
};
