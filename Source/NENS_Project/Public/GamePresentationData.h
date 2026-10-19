#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePresentationData.generated.h"
class USoundBase;
class UEndGameWidget;

/** Art, sound and pacing for the demo; designers can replace these without changing code. */
UCLASS(BlueprintType)
class NENS_PROJECT_API UGamePresentationData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Levels") TSoftObjectPtr<UWorld> MainMenuMap;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Levels") TSoftObjectPtr<UWorld> GameplayMap;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ending") TSubclassOf<UEndGameWidget> EndingWidget;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ambience") TObjectPtr<USoundBase> GameplayAmbient;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ambience") TObjectPtr<USoundBase> MainMenuAmbient;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ambience",meta=(ClampMin="0",ClampMax="1")) float GameplayVolume=.65f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ambience",meta=(ClampMin="0",ClampMax="1")) float MainMenuVolume=.7f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ambience",meta=(ClampMin="0.1",ClampMax="5")) float AudioFadeSeconds=2.f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ending",meta=(ClampMin="0",ClampMax="5")) float EndingDelaySeconds=1.2f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Transition",meta=(ClampMin="0.1",ClampMax="3")) float ScreenFadeSeconds=.75f;
    bool Validate() const;
};
