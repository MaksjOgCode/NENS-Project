#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComputerRecoveryData.h"
#include "MinigameComponent.generated.h"
class UMinigameWidget;
class UInteractionSessionSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMinigameSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMinigameClosed, bool, bCompleted);

/** Per-object success state. Each abandoned attempt gets a fresh widget. */
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NENS_PROJECT_API UMinigameComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UMinigameComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Minigame") TSubclassOf<UMinigameWidget> WidgetClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery") EComputerSystem RepairedSystem = EComputerSystem::None;
    UPROPERTY(BlueprintAssignable, Category="Minigame") FMinigameSucceeded OnSucceeded;
    UPROPERTY(BlueprintAssignable, Category="Minigame") FMinigameClosed OnMinigameClosed;
    UFUNCTION(BlueprintCallable, Category="Minigame") bool OpenMinigame(AActor* Interactor);
    UFUNCTION(BlueprintPure, Category="Minigame") bool IsComplete() const { return bCompleted; }
    bool ConfirmSuccess(UMinigameWidget* Reporter);
    void CloseMinigame(UMinigameWidget* Requester);
    UMinigameWidget* GetActiveWidget() const { return ActiveWidget; }
private:
    virtual void BeginPlay() override;
    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Minigame", meta=(AllowPrivateAccess="true")) bool bCompleted = false;
    UPROPERTY(Transient) TObjectPtr<UMinigameWidget> ActiveWidget;
    UPROPERTY(Transient) TWeakObjectPtr<UInteractionSessionSubsystem> ActiveSession;
    void SessionClosed();
};
