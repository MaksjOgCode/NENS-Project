#pragma once
#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "ComputerRecoveryWidget.generated.h"
class UComputerRecoverySubsystem;
class UTextBlock;
class UFont;
class USizeBox;

UCLASS()
class NENS_PROJECT_API UComputerRecoveryWidget : public UMinigameWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual void NativeDestruct() override;
    virtual bool CanReportSuccess() const override { return false; }
private:
    UPROPERTY(Transient) TWeakObjectPtr<UComputerRecoverySubsystem> Progress;
    float DisplayedProgress = 0;
    float ReadyOpacity[5] = {1,1,1,1,1};
    bool LastReady[5] = {false,false,false,false,false};
    UFUNCTION() void Refresh();
    UFUNCTION() void LoadCards();
    UFUNCTION() void RestoreComputer();
};

/** Compact, non-interactive objective. It fades away while a modal is open. */
UCLASS()
class NENS_PROJECT_API URecoveryObjectiveWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    URecoveryObjectiveWidget(const FObjectInitializer& Init);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY(Transient) TObjectPtr<UFont> Font;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Frame;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Summary;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Notice;
    UPROPERTY(Transient) TWeakObjectPtr<UComputerRecoverySubsystem> Progress;
    float NoticeAge = 10;
    UFUNCTION() void Changed();
};
