#include "GameExperienceSubsystem.h"
#include "GamePresentationData.h"
#include "EndGameWidget.h"
#include "ComputerRecoverySubsystem.h"
#include "InteractionSessionSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"

namespace GameExperience
{
FString MapName(const UWorld* World) { return World ? UGameplayStatics::GetCurrentLevelName(World,true) : FString(); }
bool IsMap(const UWorld* World,const TSoftObjectPtr<UWorld>& Map) { return World && MapName(World)==Map.GetAssetName(); }
UInteractionSessionSubsystem* Session(UWorld* World)
{
    auto* PC=World ? World->GetFirstPlayerController() : nullptr;
    auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    return LP ? LP->GetSubsystem<UInteractionSessionSubsystem>() : nullptr;
}
}
void UGameExperienceInstance::Init()
{
    Super::Init(); GetSubsystem<UGameExperienceSubsystem>()->Configure(Presentation);
}
void UGameExperienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection); Collection.InitializeDependency<UComputerRecoverySubsystem>();
    GetGameInstance()->GetSubsystem<UComputerRecoverySubsystem>()->OnComputerRestored.AddUniqueDynamic(this,&ThisClass::ComputerRestored);
    Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&ThisClass::Tick));
}
void UGameExperienceSubsystem::Configure(UGamePresentationData* Data) { if (Data && Data->Validate()) Settings=Data; }
void UGameExperienceSubsystem::WorldBegan(UWorld* World)
{
    if (!Settings || !World || World->GetGameInstance()!=GetGameInstance() || World->GetNetMode()==NM_DedicatedServer) return;
    CurrentWorld=World; StartAmbience(World);
    if (Phase==EPhase::Loading)
    {
        Phase=EPhase::Arriving; Age=0; EnsureCurtain(1);
    }
}
void UGameExperienceSubsystem::WorldEnded(UWorld* World)
{
    if (CurrentWorld!=World) return;
    if (auto* S=GameExperience::Session(World)) if (Ending && S->IsActive(Ending)) S->Close(Ending);
    Ending=nullptr; CurrentWorld.Reset();
    GetGameInstance()->GetSubsystem<UComputerRecoverySubsystem>()->HideObjective();
    if (Phase!=EPhase::Loading) { Phase=EPhase::Idle; Age=0; RemoveCurtain(); }
}
void UGameExperienceSubsystem::StartAmbience(UWorld* World)
{
    const bool Menu=GameExperience::IsMap(World,Settings->MainMenuMap);
    USoundBase* Sound=Menu ? Settings->MainMenuAmbient : Settings->GameplayAmbient;
    const float Volume=Menu ? Settings->MainMenuVolume : Settings->GameplayVolume;
    if (IsValid(Ambient) && Ambient->Sound==Sound && Ambient->IsPlaying())
    {
        Ambient->AdjustVolume(Settings->AudioFadeSeconds,Volume,EAudioFaderCurve::Sin); return;
    }
    if (IsValid(Ambient)) { Ambient->FadeOut(Settings->AudioFadeSeconds,0,EAudioFaderCurve::Sin); RetiringAudio.Add(Ambient); }
    Ambient=UGameplayStatics::CreateSound2D(World,Sound,1,1,0,nullptr,true,true);
    if (Ambient)
    {
        Ambient->bIsUISound=true;
        Ambient->FadeIn(Settings->AudioFadeSeconds,Volume,0,EAudioFaderCurve::Sin); ++AmbientStarts;
    }
}
bool UGameExperienceSubsystem::IsTransitioning() const { return Phase!=EPhase::Idle && Phase!=EPhase::EndingDelay; }
void UGameExperienceSubsystem::EnsureCurtain(float Opacity)
{
    if (!Curtain)
    {
        Curtain=CreateWidget<UGameScreenFadeWidget>(GetGameInstance(),UGameScreenFadeWidget::StaticClass());
        if (Curtain) { Curtain->AddToViewport(1000); if (auto* PC=CurrentWorld.IsValid() ? CurrentWorld->GetFirstPlayerController() : nullptr) Curtain->SetUserFocus(PC); }
    }
    if (Curtain) Curtain->SetRenderOpacity(Opacity);
}
void UGameExperienceSubsystem::RemoveCurtain() { if (Curtain) Curtain->RemoveFromParent(); Curtain=nullptr; }
void UGameExperienceSubsystem::ComputerRestored()
{
    if (!Settings || bEndingShown || Phase!=EPhase::Idle || !GameExperience::IsMap(CurrentWorld.Get(),Settings->GameplayMap)) return;
    Phase=EPhase::EndingDelay; Age=0;
}
void UGameExperienceSubsystem::ShowEnding()
{
    UWorld* World=CurrentWorld.Get(); auto* S=GameExperience::Session(World);
    auto* PC=World ? World->GetFirstPlayerController() : nullptr;
    if (!PC || !S) { Phase=EPhase::Idle; RemoveCurtain(); return; }
    // The curtain is opaque before replacing a terminal/inspection session.
    if (auto* Active=S->GetActiveWidget()) S->Close(Active);
    Ending=CreateWidget<UEndGameWidget>(PC,Settings->EndingWidget);
    if (!Ending) { Phase=EPhase::Idle; RemoveCurtain(); return; }
    Ending->InitializePresentationSession(FSimpleDelegate::CreateUObject(this,&ThisClass::EndingClosed));
    if (!S->Open(Ending,PC,FSimpleDelegate::CreateUObject(this,&ThisClass::EndingClosed))) { Ending=nullptr; Phase=EPhase::Idle; RemoveCurtain(); return; }
    bEndingShown=true; Phase=EPhase::EndingIn; Age=0;
}
void UGameExperienceSubsystem::EndingClosed()
{
    if (auto* S=GameExperience::Session(CurrentWorld.Get())) if (Ending && S->IsActive(Ending)) S->Close(Ending);
    Ending=nullptr;
}
bool UGameExperienceSubsystem::Navigate(EDestination Target)
{
    if (!Settings || !CurrentWorld.IsValid() || IsTransitioning() || Phase==EPhase::EndingDelay) return false;
    Destination=Target; Phase=EPhase::NavigateOut; Age=0;
    StartOpacity=Curtain ? Curtain->GetRenderOpacity() : 0; EnsureCurtain(StartOpacity);
    if (IsValid(Ambient)) Ambient->AdjustVolume(Settings->ScreenFadeSeconds,0,EAudioFaderCurve::Sin);
    return true;
}
bool UGameExperienceSubsystem::StartNewGame() { return Navigate(EDestination::NewGame); }
bool UGameExperienceSubsystem::ReturnToMainMenu() { return Navigate(EDestination::Menu); }
bool UGameExperienceSubsystem::QuitGame() { return Navigate(EDestination::Quit); }
bool UGameExperienceSubsystem::Tick(float Dt)
{
    RetiringAudio.RemoveAll([](UAudioComponent* C) { return !IsValid(C) || !C->IsPlaying(); });
    if (!Settings || Phase==EPhase::Idle) return true;
    Age+=FMath::Max(0.f,Dt);
    const float T=FMath::Clamp(Age/Settings->ScreenFadeSeconds,0.f,1.f),Smooth=T*T*(3-2*T);
    switch (Phase)
    {
    case EPhase::EndingDelay:
        if (Age>=Settings->EndingDelaySeconds) { EnsureCurtain(0); Phase=EPhase::EndingOut; Age=0; }
        break;
    case EPhase::EndingOut:
        if (Curtain) Curtain->SetRenderOpacity(Smooth);
        if (T>=1) ShowEnding();
        break;
    case EPhase::EndingIn: case EPhase::Arriving:
        if (Curtain) Curtain->SetRenderOpacity(1-Smooth);
        if (T>=1) { RemoveCurtain(); Phase=EPhase::Idle; if (Ending) Ending->SetUserFocus(Ending->GetOwningPlayer()); }
        break;
    case EPhase::NavigateOut:
        if (Curtain) Curtain->SetRenderOpacity(FMath::Lerp(StartOpacity,1.f,Smooth));
        if (T>=1)
        {
            UWorld* World=CurrentWorld.Get(); if (!World) break;
            Phase=EPhase::Loading; Age=0;
            if (Destination==EDestination::Quit) { UKismetSystemLibrary::QuitGame(World,World->GetFirstPlayerController(),EQuitPreference::Quit,false); break; }
            if (Destination==EDestination::NewGame)
            {
                bEndingShown=false; GetGameInstance()->GetSubsystem<UComputerRecoverySubsystem>()->ResetForNewGame();
            }
            UGameplayStatics::OpenLevelBySoftObjectPtr(World,Destination==EDestination::Menu ? Settings->MainMenuMap : Settings->GameplayMap);
        }
        break;
    case EPhase::Loading:
        // A failed travel must not leave an opaque, input-blocking screen forever.
        if (Age>15 && CurrentWorld.IsValid()) { StartAmbience(CurrentWorld.Get()); Phase=EPhase::Arriving; Age=0; }
        break;
    default: break;
    }
    return true;
}
void UGameExperienceSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
    GetGameInstance()->GetSubsystem<UComputerRecoverySubsystem>()->OnComputerRestored.RemoveDynamic(this,&ThisClass::ComputerRestored);
    EndingClosed(); RemoveCurtain();
    if (IsValid(Ambient)) { Ambient->Stop(); if (IsValid(Ambient)) Ambient->DestroyComponent(); } Ambient=nullptr;
    for (UAudioComponent* C : RetiringAudio) if (IsValid(C)) { C->Stop(); if (IsValid(C)) C->DestroyComponent(); }
    RetiringAudio.Reset(); CurrentWorld.Reset(); Settings=nullptr; Super::Deinitialize();
}
void UGamePresentationWorldSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    if (auto* GI=World.GetGameInstance()) GI->GetSubsystem<UGameExperienceSubsystem>()->WorldBegan(&World);
}
void UGamePresentationWorldSubsystem::OnWorldEndPlay(UWorld& World)
{
    if (auto* GI=World.GetGameInstance()) GI->GetSubsystem<UGameExperienceSubsystem>()->WorldEnded(&World);
    Super::OnWorldEndPlay(World);
}
