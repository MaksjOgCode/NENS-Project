#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ComputerRecoveryData.h"
#include "ComputerRecoverySubsystem.generated.h"
class URecoveryObjectiveWidget;
class APlayerController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRecoveryProgressChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComputerRestored);

USTRUCT()
struct FRecoveryInventoryItem
{
    GENERATED_BODY()
    UPROPERTY() FName PickupID;
    UPROPERTY() FName ItemID;
    UPROPERTY() bool bConsumed = false;
};

/** One recovery goal and a deduplicated item bag for the current game run. */
UCLASS()
class NENS_PROJECT_API UComputerRecoverySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    bool Configure(UComputerRecoveryData* Data);
    static UComputerRecoverySubsystem* From(const UObject* Context);
    static FString PlacementKey(const UActorComponent* Source,FName PickupID = NAME_None);
    bool CollectItem(const FString& SourceKey,FName PickupID,FName ItemID);
    bool HasCollected(const FString& SourceKey) const { return Inventory.Contains(SourceKey); }
    bool MarkSystemReady(EComputerSystem System,const FString& SourceKey);
    bool HasCompletedSource(const FString& SourceKey) const { return CompletedSources.Contains(SourceKey); }
    UFUNCTION(BlueprintPure, Category="Recovery") bool IsSystemReady(EComputerSystem System) const { return System != EComputerSystem::None && ReadySystems.Contains(System); }
    UFUNCTION(BlueprintPure, Category="Recovery") int32 GetReadySystemCount() const { return ReadySystems.Num(); }
    static constexpr int32 RequiredSystemCount=4;
    UFUNCTION(BlueprintPure, Category="Recovery") int32 GetInventoryCount(FName ItemID) const;
    UFUNCTION(BlueprintPure, Category="Recovery") int32 GetRequiredCardCount() const { return RequiredCards.Num(); }
    UFUNCTION(BlueprintPure, Category="Recovery") int32 GetLoadedCardCount() const { return LoadedCards.Num(); }
    UFUNCTION(BlueprintPure, Category="Recovery") int32 GetLoadableCardCount() const;
    UFUNCTION(BlueprintCallable, Category="Recovery") int32 LoadProgramCards();
    UFUNCTION(BlueprintPure, Category="Recovery") bool CanRestore() const;
    UFUNCTION(BlueprintCallable, Category="Recovery") bool BeginRestoration();
    UFUNCTION(BlueprintPure, Category="Recovery") bool IsRestoring() const { return bRestoring; }
    UFUNCTION(BlueprintPure, Category="Recovery") bool IsRestored() const { return bRestored; }
    UFUNCTION(BlueprintPure, Category="Recovery") float GetRestoreProgress() const;
    UPROPERTY(BlueprintAssignable, Category="Recovery") FRecoveryProgressChanged OnProgressChanged;
    UPROPERTY(BlueprintAssignable, Category="Recovery") FComputerRestored OnComputerRestored;
    FText GetLatestNotice() const { return LatestNotice; }
    FText GetNextStep() const;
    void ShowObjective(APlayerController* PC);
    virtual void Deinitialize() override;
private:
    UPROPERTY(Transient) TObjectPtr<UComputerRecoveryData> Definition;
    UPROPERTY(Transient) TMap<FString,FRecoveryInventoryItem> Inventory;
    UPROPERTY(Transient) TSet<FName> RequiredCards;
    UPROPERTY(Transient) TSet<FName> LoadedCards;
    UPROPERTY(Transient) TSet<EComputerSystem> ReadySystems;
    UPROPERTY(Transient) TSet<FString> CompletedSources;
    UPROPERTY(Transient) TObjectPtr<URecoveryObjectiveWidget> Objective;
    FTimerHandle RestoreTimer;
    bool bRestoring = false, bRestored = false;
    FText LatestNotice;
    void FinishRestoration();
    void Changed(const FText& Notice);
};
