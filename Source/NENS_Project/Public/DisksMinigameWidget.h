#pragma once
#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "DisksMinigameWidget.generated.h"
class UWidgetSwitcher;

/** One modal/session: inspect the room, click its cabinet, then solve the existing disks. */
UCLASS()
class NENS_PROJECT_API UDisksMinigameWidget : public UMinigameWidget
{
    GENERATED_BODY()
public:
    virtual bool IsInteractive() const override;
    bool IsShowingInspection() const;
    bool IsChangingStage() const { return StageTransition != 0; }
    /** Event coordinates remain correct after DPI scaling and viewport resize. */
    UFUNCTION(BlueprintPure, Category="Disks") FVector2D GetDiskPressPosition() const { return PressPosition; }
    UFUNCTION(BlueprintCallable, Category="Disks") void CompleteDiskPuzzle();
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Transition", meta=(ClampMin="0.05",ClampMax="1")) float StageFadeSeconds = .2f;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual bool CanReportSuccess() const override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
    FVector2D PressPosition = FVector2D::ZeroVector;
    int32 StageTransition = 0;
    float StageAge = 0, SuccessAge = 0;
    float SnapFrom[3] = {};
    void SetPageCopy();
    void StopTurning();
    UFUNCTION() void OpenCabinet();
    UFUNCTION() void ExitPuzzle();
};
