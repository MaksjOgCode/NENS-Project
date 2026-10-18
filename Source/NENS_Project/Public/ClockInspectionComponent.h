#pragma once
#include "CoreMinimal.h"
#include "SearchableComponent.h"
#include "ClockPuzzleData.h"
#include "ClockInspectionComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClockPuzzleSolved);

/** Puzzle and pickup progress live on the actor, independently of the temporary UI. */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NENS_PROJECT_API UClockInspectionComponent : public USearchableComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") TObjectPtr<UClockPuzzleData> Puzzle;
    UPROPERTY(BlueprintAssignable, Category="Clock") FClockPuzzleSolved OnPuzzleSolved;
    UFUNCTION(BlueprintPure, Category="Clock") bool IsPuzzleSolved() const { return bSolved; }
    UFUNCTION(BlueprintPure, Category="Clock") bool IsHandPlaced(EClockHand Hand) const;
    UFUNCTION(BlueprintPure, Category="Clock") float GetHandAngle(EClockHand Hand) const;
    UFUNCTION(BlueprintCallable, Category="Clock") bool PlaceHand(EClockHand Hand);
    UFUNCTION(BlueprintCallable, Category="Clock") bool RotateHand(EClockHand Hand, float Angle);
    bool ValidateSetup(FString& Error) const;
    virtual bool TryCollect(FName PickupID) override;
private:
    UPROPERTY(Transient, VisibleInstanceOnly, Category="Clock") bool bSolved = false;
    UPROPERTY(Transient, VisibleInstanceOnly, Category="Clock") bool bShortPlaced = false;
    UPROPERTY(Transient, VisibleInstanceOnly, Category="Clock") bool bLongPlaced = false;
    UPROPERTY(Transient) float ShortAngle = 0;
    UPROPERTY(Transient) float LongAngle = 0;
    UPROPERTY(Transient) bool bAnglesInitialized = false;
    void InitializeAngles();
    void CheckSolution();
};
