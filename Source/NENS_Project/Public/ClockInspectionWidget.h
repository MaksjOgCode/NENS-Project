#pragma once
#include "CoreMinimal.h"
#include "InspectionSceneWidget.h"
#include "ClockPuzzleData.h"
#include "ClockInspectionWidget.generated.h"
class UClockInspectionComponent;
class UInspectionSubsystem;
class UCanvasPanel;
class UImage;
class UButton;
class UTextBlock;
class UBorder;

/** One shared inspection session, with a mouse-driven puzzle and a crossfade to normal pickups. */
UCLASS()
class NENS_PROJECT_API UClockInspectionWidget : public UInspectionSceneWidget
{
    GENERATED_BODY()
public:
    virtual bool Configure(USearchableComponent* Source, UInspectionSubsystem* Session) override;
    UFUNCTION(BlueprintPure, Category="Clock") bool IsTransitioning() const { return bTransitioning; }
    UFUNCTION(BlueprintPure, Category="Clock") bool IsCompartmentVisible() const { return bCompartmentVisible; }
    UFUNCTION(BlueprintCallable, Category="Clock") void SelectHand(EClockHand Hand);
    UFUNCTION(BlueprintPure, Category="Clock") EClockHand GetSelectedHand() const { return Selected; }
    FVector2D HandAnchor(EClockHand Hand) const;
    FVector2D HandTip(EClockHand Hand) const;
    UCanvasPanel* GetSceneCanvas() const { return SceneCanvas; }
protected:
    virtual bool UsesDefaultInspectionHints() const override { return false; }
    virtual void OnCloseStarted() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnMouseMove(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent&) override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<UClockInspectionComponent> Clock;
    UPROPERTY(Transient) TWeakObjectPtr<UInspectionSubsystem> Session;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SceneCanvas;
    UPROPERTY(Transient) TObjectPtr<UImage> ClosedImage;
    UPROPERTY(Transient) TObjectPtr<UImage> OpenImage;
    UPROPERTY(Transient) TObjectPtr<UImage> ShortImage;
    UPROPERTY(Transient) TObjectPtr<UImage> LongImage;
    UPROPERTY(Transient) TObjectPtr<UButton> ShortButton;
    UPROPERTY(Transient) TObjectPtr<UButton> LongButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Instructions;
    UPROPERTY(Transient) TObjectPtr<UBorder> InstructionPanel;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> TrayPanels;
    enum class EDrag : uint8 { None, FromTray, Rotate };
    EDrag Drag = EDrag::None;
    EClockHand Selected = EClockHand::Short;
    FVector2D CursorScene, GrabOffset;
    float PreviewAngle = 0, TransitionTime = 0;
    float HintWidth = 0;
    bool bTransitioning = false, bCompartmentVisible = false;
    bool IsCurrent() const;
    bool IsPuzzleInteractive() const;
    bool HitHand(FVector2D Point, EClockHand Hand) const;
    float DisplayAngle(EClockHand Hand) const;
    void RefreshHands();
    void ShowCompartment();
    void BeginTransition();
    void RotateSelected(float Delta);
    UFUNCTION() void SelectShort();
    UFUNCTION() void SelectLong();
};
