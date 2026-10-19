#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArithmeticPuzzleData.generated.h"
class UTexture2D;

/** Aligned, cumulative equations. Original PNGs remain unchanged. */
UCLASS(BlueprintType)
class NENS_PROJECT_API UArithmeticPuzzleData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Arithmetic") TArray<TObjectPtr<UTexture2D>> Frames;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Arithmetic") int32 Answer=29;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Arithmetic",meta=(ClampMin="0.1",ClampMax="5")) float HoldSeconds=.85f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Arithmetic",meta=(ClampMin="0.1",ClampMax="3")) float BlendSeconds=.65f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Arithmetic",meta=(ClampMin="0.1",ClampMax="2")) float InputFadeSeconds=.35f;
    bool Validate() const;
    bool IsCorrect(const FString& Text) const;
};
