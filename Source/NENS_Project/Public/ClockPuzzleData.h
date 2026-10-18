#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClockPuzzleData.generated.h"
class UTexture2D;
class UFont;

UENUM(BlueprintType)
enum class EClockHand : uint8 { Short, Long };

/** Art calibration in scene pixels. UVs crop transparent PNG margins without editing the source. */
USTRUCT(BlueprintType)
struct NENS_PROJECT_API FClockHandArt
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> Texture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D UVMin = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D UVMax = FVector2D(1,1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D Pivot = FVector2D(.5,.5);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D Size = FVector2D(40,100);
    /** Clockwise angle from XII already drawn in the texture. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float NativeAngle = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TipLength = 90;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D TrayPosition = FVector2D(350,750);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float InitialAngle = 210;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TargetAngle = 0;
};

UCLASS(BlueprintType)
class NENS_PROJECT_API UClockPuzzleData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") TObjectPtr<UTexture2D> ClosedBackground;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") FVector2D CanvasSize = FVector2D(1875,1902);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") FVector2D DialCenter = FVector2D(938,479);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") float DialRadius = 118;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") FClockHandArt ShortHand;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock") FClockHandArt LongHand;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock|Interface") TObjectPtr<UFont> InterfaceFont;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock|Interface", meta=(ClampMin="1",ClampMax="4")) float TrayScale = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock", meta=(ClampMin="1",ClampMax="15")) float AngleTolerance = 7;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock", meta=(ClampMin="1",ClampMax="30")) float SnapDegrees = 6;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clock", meta=(ClampMin="0.1",ClampMax="3")) float TransitionSeconds = 1;
    const FClockHandArt& Hand(EClockHand Which) const { return Which == EClockHand::Short ? ShortHand : LongHand; }
    bool Validate(FString& Error) const;
};
