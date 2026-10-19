#include "ArithmeticPuzzleData.h"
#include "Engine/Texture2D.h"
bool UArithmeticPuzzleData::Validate() const
{
    if (Frames.Num()!=4 || Answer<0 || Answer>9999) return false;
    for (const auto& Frame : Frames) if (!Frame) return false;
    return FMath::IsFinite(HoldSeconds) && HoldSeconds>=.1f && HoldSeconds<=5
        && FMath::IsFinite(BlendSeconds) && BlendSeconds>=.1f && BlendSeconds<=3
        && FMath::IsFinite(InputFadeSeconds) && InputFadeSeconds>=.1f && InputFadeSeconds<=2;
}
bool UArithmeticPuzzleData::IsCorrect(const FString& Text) const
{
    const FString Number=Text.TrimStartAndEnd();
    if (Number.IsEmpty() || Number.Len()>4) return false;
    for (TCHAR C : Number) if (C<TEXT('0') || C>TEXT('9')) return false;
    int32 Value=0; return LexTryParseString(Value,*Number) && Value==Answer;
}
