#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "InspectionSceneWidget.generated.h"
class USearchableComponent;
class UInspectionSubsystem;
class UInspectionSceneWidget;
class UCanvasPanel;
class UBorder;
class UTextBlock;
class UFont;
class SInspectionMist;

UCLASS()
class NENS_PROJECT_API UInspectionPickupButton : public UButton
{
    GENERATED_BODY()
public:
    UInspectionPickupButton(const FObjectInitializer& ObjectInitializer);
    void Configure(UInspectionSceneWidget* Scene, FName ID);
    FName GetPickupID() const { return PickupID; }
    void BeginCollectionFade(float Seconds);
    void AdvanceCollectionFade(float DeltaTime);
    void FinishCollectionFade();
    bool IsCollectionFading() const { return bCollectionFading; }
private:
    UPROPERTY(Transient) TWeakObjectPtr<UInspectionSceneWidget> SceneWidget;
    FName PickupID;
    float FadeTime = 0, FadeDuration = 0;
    bool bCollectionFading = false;
    UFUNCTION() void Collect();
};

/** Behaviour for the existing WBP_InspectionScene designer layout. */
UCLASS()
class NENS_PROJECT_API UInspectionSceneWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInspectionSceneWidget(const FObjectInitializer& ObjectInitializer);
    virtual bool Configure(USearchableComponent* Source, UInspectionSubsystem* Session);
    UFUNCTION(BlueprintCallable, Category="Inspection") bool CollectPickup(FName PickupID);
    UFUNCTION(BlueprintPure, Category="Inspection") bool IsOpening() const { return bOpening; }
    UFUNCTION(BlueprintPure, Category="Inspection") bool IsClosing() const { return bClosing; }
    UFUNCTION(BlueprintPure, Category="Inspection") bool IsInspectionInteractive() const;
    void BeginClose();
    const TArray<TObjectPtr<UInspectionPickupButton>>& GetPickupButtons() const { return PickupButtons; }
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Appearance") TObjectPtr<UFont> InterfaceFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Appearance", meta=(ClampMin="0",ClampMax="2")) float PickupFadeSeconds = .4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Appearance", meta=(ClampMin="0",ClampMax="2")) float SceneFadeInSeconds = .3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Appearance", meta=(ClampMin="0",ClampMax="2")) float SceneFadeOutSeconds = .24f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Mist", meta=(ClampMin="0",ClampMax="1")) float MistOpacity = .42f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Mist", meta=(ClampMin="0",ClampMax="2")) float MistSpeed = .35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inspection|Mist", meta=(ClampMin="0.05",ClampMax="0.5")) float MistHeight = .28f;
    virtual void OnCloseStarted() {}
    virtual bool UsesDefaultInspectionHints() const { return true; }
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<USearchableComponent> SourceComponent;
    UPROPERTY(Transient) TWeakObjectPtr<UInspectionSubsystem> InspectionSession;
    UPROPERTY(Transient) TArray<TObjectPtr<UInspectionPickupButton>> PickupButtons;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> InspectionCanvas;
    UPROPERTY(Transient) TObjectPtr<UBorder> HelpPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpText;
    FVector2D LastHelpViewport = FVector2D::ZeroVector;
    float LastSideMargin = -1;
    bool bSceneHasPickups = false;
    bool bOpening = true, bClosing = false;
    float PresentationTime = 0, ClosingOpacity = 1, ClosingDuration = 0, MistTime = 0;
    TSharedPtr<SInspectionMist> Mist;
    void UpdateHelpLayout(const FGeometry& Geometry);
};
