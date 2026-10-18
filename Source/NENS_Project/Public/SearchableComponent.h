#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SearchableComponent.generated.h"

class UInspectionSceneWidget;

class UInspectionSceneData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInspectionItemCollected, FName, PickupID, FName, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInspectionClosed, bool, bCompleted);

/** Per-object, current-session inspection progress. No inventory or save-game dependency. */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NENS_PROJECT_API USearchableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USearchableComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    TObjectPtr<UInspectionSceneData> Scene = nullptr;

    /** Explicit asset reference also includes the widget in cooked builds. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inspection")
    TSubclassOf<UInspectionSceneWidget> SceneWidgetClass;

    /** Emitted only after a previously uncollected placement has been recorded. */
    UPROPERTY(BlueprintAssignable, Category="Inspection")
    FInspectionItemCollected OnItemCollected;

    /** The owner may hide its world prompt once a completed inspection has closed. */
    UPROPERTY(BlueprintAssignable, Category="Inspection")
    FInspectionClosed OnInspectionClosed;

    UFUNCTION(BlueprintPure, Category="Inspection")
    bool IsComplete() const;

    UFUNCTION(BlueprintPure, Category="Inspection")
    bool IsCollected(FName PickupID) const;

    UFUNCTION(BlueprintCallable, Category="Inspection")
    virtual bool TryCollect(FName PickupID);

    UFUNCTION(BlueprintCallable, Category="Inspection")
    void OpenInspection(AActor* Interactor);

    /** Called by the inspection session after its UI has been removed. */
    void NotifyInspectionClosed();

private:
    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Inspection", meta=(AllowPrivateAccess="true"))
    TSet<FName> CollectedPickupIDs;
};
