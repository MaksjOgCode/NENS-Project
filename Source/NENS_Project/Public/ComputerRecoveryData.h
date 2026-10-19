#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ComputerRecoveryData.generated.h"
class UInspectionSceneData;

UENUM(BlueprintType)
enum class EComputerSystem : uint8 { None, Power, Fuel, Display };

/** The program is defined by the actual collectible placements, rather than a magic card count. */
UCLASS(BlueprintType)
class NENS_PROJECT_API UComputerRecoveryData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") TArray<TObjectPtr<UInspectionSceneData>> CardScenes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") FName ProgramItemID = TEXT("PunchCard");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery", meta=(ClampMin="0.2",ClampMax="20")) float RestoreSeconds = 3.f;
    bool GetRequiredCards(TSet<FName>& OutCards) const;
};
