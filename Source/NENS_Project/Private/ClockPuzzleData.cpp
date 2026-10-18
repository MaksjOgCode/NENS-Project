#include "ClockPuzzleData.h"
#include "Engine/Texture2D.h"

bool UClockPuzzleData::Validate(FString& Error) const
{
    Error.Reset();
    if (!ClosedBackground || CanvasSize.ContainsNaN() || CanvasSize.X <= 0 || CanvasSize.Y <= 0
        || DialCenter.ContainsNaN() || DialCenter.X < 0 || DialCenter.Y < 0
        || DialCenter.X > CanvasSize.X || DialCenter.Y > CanvasSize.Y || !FMath::IsFinite(DialRadius) || DialRadius <= 0
        || !FMath::IsFinite(AngleTolerance) || AngleTolerance <= 0 || AngleTolerance >= 30
        || !FMath::IsFinite(SnapDegrees) || SnapDegrees <= 0 || SnapDegrees > 30
        || !FMath::IsFinite(TransitionSeconds) || TransitionSeconds <= 0
        || !FMath::IsFinite(TrayScale) || TrayScale < 1 || TrayScale > 4)
    { Error = TEXT("Invalid clock background, dial geometry or interaction settings."); return false; }
    for (const FClockHandArt* Art : {&ShortHand, &LongHand})
    {
        if (!Art->Texture || Art->Size.ContainsNaN() || Art->Size.X <= 0 || Art->Size.Y <= 0
            || Art->Pivot.ContainsNaN() || Art->Pivot.X < 0 || Art->Pivot.X > 1 || Art->Pivot.Y < 0 || Art->Pivot.Y > 1
            || Art->UVMin.ContainsNaN() || Art->UVMax.ContainsNaN()
            || Art->UVMin.X < 0 || Art->UVMin.Y < 0 || Art->UVMax.X > 1 || Art->UVMax.Y > 1
            || Art->UVMin.X >= Art->UVMax.X || Art->UVMin.Y >= Art->UVMax.Y
            || Art->TrayPosition.ContainsNaN() || !FMath::IsFinite(Art->TipLength) || Art->TipLength <= 0
            || !FMath::IsFinite(Art->NativeAngle) || !FMath::IsFinite(Art->InitialAngle) || !FMath::IsFinite(Art->TargetAngle))
        { Error = TEXT("Invalid clock hand texture or art calibration."); return false; }
    }
    return true;
}
