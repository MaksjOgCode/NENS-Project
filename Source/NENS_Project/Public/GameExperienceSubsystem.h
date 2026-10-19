#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "GameExperienceSubsystem.generated.h"
class UGamePresentationData;
class UEndGameWidget;
class UGameScreenFadeWidget;
class UAudioComponent;

UCLASS()
class NENS_PROJECT_API UGameExperienceInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Presentation") TObjectPtr<UGamePresentationData> Presentation;
    virtual void Init() override;
};

/** One ambient voice across travel, one ending, and transitions that cover synchronous level loading. */
UCLASS()
class NENS_PROJECT_API UGameExperienceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void Configure(UGamePresentationData* Settings);
    void WorldBegan(UWorld* World);
    void WorldEnded(UWorld* World);
    UFUNCTION(BlueprintCallable,Category="Game") bool StartNewGame();
    UFUNCTION(BlueprintCallable,Category="Game") bool ReturnToMainMenu();
    UFUNCTION(BlueprintCallable,Category="Game") bool QuitGame();
    UFUNCTION(BlueprintPure,Category="Game") bool IsTransitioning() const;
    UEndGameWidget* GetEndingWidget() const { return Ending; }
    UAudioComponent* GetAmbientComponent() const { return Ambient; }
    UGamePresentationData* GetSettings() const { return Settings; }
    int32 GetAmbientStartCount() const { return AmbientStarts; }
private:
    enum class EPhase : uint8 { Idle,EndingDelay,EndingOut,EndingIn,NavigateOut,Loading,Arriving };
    enum class EDestination : uint8 { Menu,NewGame,Quit };
    UPROPERTY(Transient) TObjectPtr<UGamePresentationData> Settings;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Ambient;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> RetiringAudio;
    UPROPERTY(Transient) TObjectPtr<UEndGameWidget> Ending;
    UPROPERTY(Transient) TObjectPtr<UGameScreenFadeWidget> Curtain;
    UPROPERTY(Transient) TWeakObjectPtr<UWorld> CurrentWorld;
    FTSTicker::FDelegateHandle Ticker;
    EPhase Phase=EPhase::Idle;
    EDestination Destination=EDestination::Menu;
    float Age=0,StartOpacity=0;
    int32 AmbientStarts=0;
    bool bEndingShown=false;
    bool Tick(float DeltaTime);
    bool Navigate(EDestination Target);
    void StartAmbience(UWorld* World);
    void EnsureCurtain(float InitialOpacity);
    void RemoveCurtain();
    void ShowEnding();
    void EndingClosed();
    UFUNCTION() void ComputerRestored();
};

/** Only Game/PIE worlds notify the persistent presentation service. */
UCLASS()
class NENS_PROJECT_API UGamePresentationWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void OnWorldEndPlay(UWorld& World) override;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override { return Type==EWorldType::Game || Type==EWorldType::PIE; }
};
