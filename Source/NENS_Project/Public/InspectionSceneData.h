#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InspectionSceneData.generated.h"

class UTexture2D;

/** One placed pickup. PickupID identifies this placement; ItemID identifies its item type. */
USTRUCT(BlueprintType)
struct NENS_PROJECT_API FInspectionPickup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    FName PickupID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    TObjectPtr<UTexture2D> Image = nullptr;

    /** Top-left corner in the scene's design pixels, before the whole scene is scaled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection", meta=(ClampMin="1.0"))
    FVector2D Size = FVector2D(128.0, 77.0);
};

/** Reusable layout only. Collection progress belongs to each object's SearchableComponent. */
UCLASS(BlueprintType)
class NENS_PROJECT_API UInspectionSceneData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    TObjectPtr<UTexture2D> Background = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection", meta=(ClampMin="1.0"))
    FVector2D CanvasSize = FVector2D(812.0, 812.0);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection", meta=(TitleProperty="PickupID"))
    TArray<FInspectionPickup> Pickups;

    /** Empty scenes are allowed, but they never count as completed. */
    UFUNCTION(BlueprintPure, Category="Inspection")
    bool ValidateScene(FString& OutError) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
