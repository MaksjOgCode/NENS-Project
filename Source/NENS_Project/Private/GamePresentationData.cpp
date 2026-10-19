#include "GamePresentationData.h"
#include "EndGameWidget.h"
#include "Sound/SoundBase.h"
bool UGamePresentationData::Validate() const
{
    return !MainMenuMap.IsNull() && !GameplayMap.IsNull() && MainMenuMap!=GameplayMap && EndingWidget
        && GameplayAmbient && MainMenuAmbient && GameplayAmbient!=MainMenuAmbient
        && FMath::IsFinite(GameplayVolume) && GameplayVolume>=0 && GameplayVolume<=1
        && FMath::IsFinite(MainMenuVolume) && MainMenuVolume>=0 && MainMenuVolume<=1
        && FMath::IsFinite(AudioFadeSeconds) && AudioFadeSeconds>=.1f && AudioFadeSeconds<=5
        && FMath::IsFinite(ScreenFadeSeconds) && ScreenFadeSeconds>=.1f && ScreenFadeSeconds<=3
        && FMath::IsFinite(EndingDelaySeconds) && EndingDelaySeconds>=0 && EndingDelaySeconds<=5;
}
