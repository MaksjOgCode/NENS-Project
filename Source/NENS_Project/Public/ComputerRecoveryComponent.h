#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComputerRecoverySubsystem.h"
#include "ComputerRecoveryComponent.generated.h"
class UComputerRecoveryWidget;
class UInteractionSessionSubsystem;
/** A repeatable terminal: completion belongs to the run, never to its temporary window. */
UCLASS(ClassGroup=(Interaction),meta=(BlueprintSpawnableComponent))
class NENS_PROJECT_API UComputerRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UComputerRecoveryComponent();
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Recovery") TObjectPtr<UComputerRecoveryData> Definition;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Recovery") TSubclassOf<UComputerRecoveryWidget> WidgetClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Recovery") bool bShowObjective = true;
    UPROPERTY(BlueprintAssignable,Category="Recovery") FComputerRestored OnRestored;
    UFUNCTION(BlueprintCallable,Category="Recovery") bool OpenComputer(AActor* Interactor);
    UComputerRecoveryWidget* GetActiveWidget() const { return ActiveWidget; }
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<UComputerRecoveryWidget> ActiveWidget;
    UPROPERTY(Transient) TWeakObjectPtr<UInteractionSessionSubsystem> ActiveSession;
    void CloseFromWidget(UComputerRecoveryWidget* Expected);
    void SessionClosed();
    UFUNCTION() void Restored();
};
