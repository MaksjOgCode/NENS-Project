#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinigameWidget.generated.h"
class UMinigameComponent;
class UFont;
class SInspectionMist;

/** Adapter contract: the Blueprint puzzle reports success and requests close. */
UCLASS()
class NENS_PROJECT_API UMinigameWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UMinigameWidget(const FObjectInitializer& ObjectInitializer);
    void InitializeSession(UMinigameComponent* Source);
    UFUNCTION(BlueprintCallable, Category="Minigame") void ReportSuccess();
    UFUNCTION(BlueprintCallable, Category="Minigame") void RequestClose();
    UFUNCTION(BlueprintPure, Category="Minigame") bool HasReportedSuccess() const { return bReportedSuccess; }
    bool IsOpening() const { return !bRemoved && !bClosing && FadeAge < SceneFadeInSeconds; }
    bool IsClosing() const { return bClosing; }
    bool IsInteractive() const { return !bRemoved && !bClosing && !IsOpening(); }
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Transition", meta=(ClampMin="0",ClampMax="2")) float SceneFadeInSeconds = .3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Transition", meta=(ClampMin="0",ClampMax="2")) float SceneFadeOutSeconds = .24f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Appearance") TObjectPtr<UFont> InterfaceFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Appearance", meta=(ClampMin="0",ClampMax="120")) float ScreenPadding = 32.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0",ClampMax="1")) float MistOpacity = .42f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0",ClampMax="2")) float MistSpeed = .35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0.05",ClampMax="0.5")) float MistHeight = .28f;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual bool CanReportSuccess() const { return true; }
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<UMinigameComponent> SourceComponent;
    bool bManagedSession = false;
    bool bReportedSuccess = false;
    bool bClosing = false;
    bool bRemoved = false;
    bool bStandaloneInput = false;
    float FadeAge = 0;
    float CloseOpacity = 1;
    float MistTime = 0;
    TSharedPtr<SInspectionMist> Mist;
    void FinishClose();
    void RestoreStandaloneInput();
};
