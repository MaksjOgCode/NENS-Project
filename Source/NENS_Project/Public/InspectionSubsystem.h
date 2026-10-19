#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "InspectionSubsystem.generated.h"
class USearchableComponent;
class UInspectionSceneWidget;
class APlayerController;
class UInteractionSessionSubsystem;

/** Owns the single modal inspection for this local player. */
UCLASS()
class NENS_PROJECT_API UInspectionSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Inspection") bool Open(USearchableComponent* Source);
    UFUNCTION(BlueprintCallable, Category="Inspection") void Close();
    /** Immediate teardown, guarded by the originating widget. Used after its fade or on destruction. */
    void FinishClose(UInspectionSceneWidget* ExpectedWidget);
    UFUNCTION(BlueprintPure, Category="Inspection") bool IsOpen() const { return ActiveWidget != nullptr; }
    UInspectionSceneWidget* GetActiveWidget() const { return ActiveWidget; }
    USearchableComponent* GetActiveSource() const { return ActiveSource.Get(); }
    virtual void Deinitialize() override;
private:
    UPROPERTY(Transient) TObjectPtr<UInspectionSceneWidget> ActiveWidget;
    UPROPERTY(Transient) TWeakObjectPtr<USearchableComponent> ActiveSource;
    UPROPERTY(Transient) TWeakObjectPtr<UInteractionSessionSubsystem> ActiveSession;
    void SessionClosed();
};
