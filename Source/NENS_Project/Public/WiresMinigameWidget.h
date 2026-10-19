#pragma once

#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "WiresMinigameWidget.generated.h"

class UFont;
class SInspectionMist;

/** Presentation for the existing Blueprint wires puzzle. Pair rules stay in Blueprint. */
UCLASS()
class NENS_PROJECT_API UWiresMinigameWidget : public UMinigameWidget
{
    GENERATED_BODY()
public:
    UWiresMinigameWidget(const FObjectInitializer& ObjectInitializer);
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Appearance") TObjectPtr<UFont> InterfaceFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Appearance", meta=(ClampMin="0",ClampMax="120")) float ScreenPadding = 32.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0",ClampMax="1")) float MistOpacity = .42f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0",ClampMax="2")) float MistSpeed = .35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minigame|Mist", meta=(ClampMin="0.05",ClampMax="0.5")) float MistHeight = .28f;
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual void NativeDestruct() override;
private:
    TSharedPtr<SInspectionMist> Mist;
    float MistTime = 0;
};
