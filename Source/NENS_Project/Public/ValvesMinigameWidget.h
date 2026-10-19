#pragma once
#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "ValvesMinigameWidget.generated.h"
class SValveIndicator;

/** A readable orientation overlay for the existing interpolated Blueprint valve. */
UCLASS()
class NENS_PROJECT_API UValveControlWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool IsAligned() const;
    double ReadAngle(FName Name) const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
private:
    TSharedPtr<SValveIndicator> Indicator;
};

/** Session/presentation adapter; the original Blueprint retains its three valve rules. */
UCLASS()
class NENS_PROJECT_API UValvesMinigameWidget : public UMinigameWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual bool CanReportSuccess() const override;
private:
    int32 AlignedCount() const;
    float SuccessAge = 0;
    int32 LastCount = -1;
};
