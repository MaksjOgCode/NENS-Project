#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinigameWidget.generated.h"
class UMinigameComponent;

/** Adapter contract: the Blueprint puzzle reports success and requests close. */
UCLASS()
class NENS_PROJECT_API UMinigameWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void InitializeSession(UMinigameComponent* Source);
    UFUNCTION(BlueprintCallable, Category="Minigame") void ReportSuccess();
    UFUNCTION(BlueprintCallable, Category="Minigame") void RequestClose();
    UFUNCTION(BlueprintPure, Category="Minigame") bool HasReportedSuccess() const { return bReportedSuccess; }
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<UMinigameComponent> SourceComponent;
    bool bManagedSession = false;
    bool bReportedSuccess = false;
};
