#include "ClockInspectionComponent.h"
#include "InspectionSceneData.h"

bool UClockInspectionComponent::ValidateSetup(FString& Error) const
{
    if (!Puzzle || !Puzzle->Validate(Error)) { if (!Puzzle) Error = TEXT("Clock Puzzle data is missing."); return false; }
    if (!Scene || !Scene->ValidateScene(Error) || Scene->Pickups.IsEmpty() || !Scene->CanvasSize.Equals(Puzzle->CanvasSize))
    { if (Error.IsEmpty()) Error = TEXT("Clock needs an open-compartment scene with pickups and the same canvas size."); return false; }
    return true;
}
bool UClockInspectionComponent::IsHandPlaced(EClockHand Hand) const { return Hand == EClockHand::Short ? bShortPlaced : bLongPlaced; }
float UClockInspectionComponent::GetHandAngle(EClockHand Hand) const
{
    return bAnglesInitialized ? (Hand == EClockHand::Short ? ShortAngle : LongAngle) : (Puzzle ? Puzzle->Hand(Hand).InitialAngle : 0);
}
void UClockInspectionComponent::InitializeAngles()
{
    if (!bAnglesInitialized && Puzzle) { ShortAngle = Puzzle->ShortHand.InitialAngle; LongAngle = Puzzle->LongHand.InitialAngle; bAnglesInitialized = true; }
}
bool UClockInspectionComponent::PlaceHand(EClockHand Hand)
{
    FString Error;
    if (bSolved || IsHandPlaced(Hand) || !ValidateSetup(Error)) return false;
    InitializeAngles();
    (Hand == EClockHand::Short ? bShortPlaced : bLongPlaced) = true;
    CheckSolution();
    return true;
}
bool UClockInspectionComponent::RotateHand(EClockHand Hand, float Angle)
{
    FString Error;
    if (bSolved || !IsHandPlaced(Hand) || !FMath::IsFinite(Angle) || !ValidateSetup(Error)) return false;
    InitializeAngles();
    (Hand == EClockHand::Short ? ShortAngle : LongAngle) = FRotator::ClampAxis(Angle);
    CheckSolution();
    return true;
}
void UClockInspectionComponent::CheckSolution()
{
    if (!bSolved && bShortPlaced && bLongPlaced
        && FMath::Abs(FMath::FindDeltaAngleDegrees(ShortAngle, Puzzle->ShortHand.TargetAngle)) <= Puzzle->AngleTolerance
        && FMath::Abs(FMath::FindDeltaAngleDegrees(LongAngle, Puzzle->LongHand.TargetAngle)) <= Puzzle->AngleTolerance)
    {
        bSolved = true; // Commit before notifying UI; Escape during the fade cannot undo this stage.
        ShortAngle = Puzzle->ShortHand.TargetAngle; LongAngle = Puzzle->LongHand.TargetAngle;
        OnPuzzleSolved.Broadcast();
    }
}
bool UClockInspectionComponent::TryCollect(FName PickupID) { return bSolved && Super::TryCollect(PickupID); }
