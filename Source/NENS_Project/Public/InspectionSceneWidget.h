#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "InspectionSceneWidget.generated.h"
class USearchableComponent;
class UInspectionSubsystem;
class UInspectionSceneWidget;
class UCanvasPanel;

UCLASS()
class NENS_PROJECT_API UInspectionPickupButton : public UButton
{
    GENERATED_BODY()
public:
    UInspectionPickupButton(const FObjectInitializer& ObjectInitializer);
    void Configure(UInspectionSceneWidget* Scene, FName ID);
    FName GetPickupID() const { return PickupID; }
private:
    UPROPERTY(Transient) TWeakObjectPtr<UInspectionSceneWidget> SceneWidget;
    FName PickupID;
    UFUNCTION() void Collect();
};

/** Behaviour for the existing WBP_InspectionScene designer layout. */
UCLASS()
class NENS_PROJECT_API UInspectionSceneWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInspectionSceneWidget(const FObjectInitializer& ObjectInitializer);
    bool Configure(USearchableComponent* Source, UInspectionSubsystem* Session);
    UFUNCTION(BlueprintCallable, Category="Inspection") bool CollectPickup(FName PickupID);
    const TArray<TObjectPtr<UInspectionPickupButton>>& GetPickupButtons() const { return PickupButtons; }
protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<USearchableComponent> SourceComponent;
    UPROPERTY(Transient) TWeakObjectPtr<UInspectionSubsystem> InspectionSession;
    UPROPERTY(Transient) TArray<TObjectPtr<UInspectionPickupButton>> PickupButtons;
};
