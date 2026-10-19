#include "ComputerRecoverySubsystem.h"
#include "ComputerRecoveryWidget.h"
#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UComputerRecoverySubsystem* UComputerRecoverySubsystem::From(const UObject* Context)
{
    auto* World=Context ? Context->GetWorld() : nullptr;
    auto* GI=World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UComputerRecoverySubsystem>() : nullptr;
}
FString UComputerRecoverySubsystem::PlacementKey(const UActorComponent* Source,FName PickupID)
{
    if (!Source || !Source->GetOwner()) return {};
    // Actor paths distinguish separate placed copies and survive unloading/reloading that placement.
    return Source->GetOwner()->GetPathName()+TEXT("|")+Source->GetFName().ToString()+TEXT("|")+PickupID.ToString();
}
bool UComputerRecoverySubsystem::Configure(UComputerRecoveryData* Data)
{
    if (!Data || (Definition && Definition != Data)) return false;
    TSet<FName> Cards;
    if (!Data->GetRequiredCards(Cards)) return false;
    if (Definition) return true;
    Definition=Data; RequiredCards=MoveTemp(Cards);
    Changed(NSLOCTEXT("Recovery","Objective","Repair the systems and find the punch cards."));
    return true;
}
void UComputerRecoverySubsystem::Changed(const FText& Notice) { LatestNotice=Notice; OnProgressChanged.Broadcast(); }
void UComputerRecoverySubsystem::HideObjective() { if (Objective) Objective->RemoveFromParent(); Objective=nullptr; }
void UComputerRecoverySubsystem::ResetForNewGame()
{
    if (GetGameInstance()) GetGameInstance()->GetTimerManager().ClearTimer(RestoreTimer);
    HideObjective(); Definition=nullptr; Inventory.Reset(); RequiredCards.Reset(); LoadedCards.Reset();
    ReadySystems.Reset(); CompletedSources.Reset(); bRestoring=false; bRestored=false; LatestNotice=FText::GetEmpty();
    OnProgressChanged.Broadcast();
}
bool UComputerRecoverySubsystem::CollectItem(const FString& SourceKey,FName PickupID,FName ItemID)
{
    if (SourceKey.IsEmpty() || PickupID.IsNone() || ItemID.IsNone() || Inventory.Contains(SourceKey)) return false;
    FRecoveryInventoryItem Item; Item.PickupID=PickupID; Item.ItemID=ItemID;
    Inventory.Add(SourceKey,Item);
    Changed(ItemID==TEXT("PunchCard") ? NSLOCTEXT("Recovery","CardFound","Punch card added to your bag") : NSLOCTEXT("Recovery","ItemFound","Item added to your bag"));
    return true;
}
int32 UComputerRecoverySubsystem::GetInventoryCount(FName ItemID) const
{
    int32 Count=0;
    for (const auto& Pair : Inventory) if (Pair.Value.ItemID==ItemID && !Pair.Value.bConsumed) ++Count;
    return Count;
}
bool UComputerRecoverySubsystem::MarkSystemReady(EComputerSystem System,const FString& SourceKey)
{
    if ((System!=EComputerSystem::Power && System!=EComputerSystem::Fuel && System!=EComputerSystem::Display && System!=EComputerSystem::Arithmetic) || SourceKey.IsEmpty() || CompletedSources.Contains(SourceKey)) return false;
    CompletedSources.Add(SourceKey);
    if (ReadySystems.Contains(System)) return false;
    ReadySystems.Add(System);
    Changed(System==EComputerSystem::Power ? NSLOCTEXT("Recovery","PowerRestored","Power restored — Main computer")
        : System==EComputerSystem::Fuel ? NSLOCTEXT("Recovery","FuelRestored","Fuel supply restored — Main computer")
        : System==EComputerSystem::Display ? NSLOCTEXT("Recovery","DisplayRestored","Display calibrated — Main computer")
        : NSLOCTEXT("Recovery","ArithmeticRestored","Arithmetic check passed — Main computer"));
    return true;
}
int32 UComputerRecoverySubsystem::GetLoadableCardCount() const
{
    TSet<FName> Available;
    for (const auto& Pair : Inventory)
        if (!Pair.Value.bConsumed && Definition && Pair.Value.ItemID==Definition->ProgramItemID && RequiredCards.Contains(Pair.Value.PickupID) && !LoadedCards.Contains(Pair.Value.PickupID)) Available.Add(Pair.Value.PickupID);
    return Available.Num();
}
int32 UComputerRecoverySubsystem::LoadProgramCards()
{
    if (!Definition || bRestoring || bRestored) return 0;
    int32 Count=0;
    for (auto& Pair : Inventory)
    {
        auto& Item=Pair.Value;
        if (!Item.bConsumed && Item.ItemID==Definition->ProgramItemID && RequiredCards.Contains(Item.PickupID) && !LoadedCards.Contains(Item.PickupID))
        { Item.bConsumed=true; LoadedCards.Add(Item.PickupID); ++Count; }
    }
    if (Count) Changed(NSLOCTEXT("Recovery","CardsLoaded","Punch cards loaded into the recovery program"));
    return Count;
}
bool UComputerRecoverySubsystem::CanRestore() const
{
    return Definition && !bRestoring && !bRestored && !RequiredCards.IsEmpty() && LoadedCards.Num()==RequiredCards.Num()
        && IsSystemReady(EComputerSystem::Power) && IsSystemReady(EComputerSystem::Fuel) && IsSystemReady(EComputerSystem::Display) && IsSystemReady(EComputerSystem::Arithmetic);
}
bool UComputerRecoverySubsystem::BeginRestoration()
{
    if (!CanRestore() || !GetGameInstance()) return false;
    bRestoring=true;
    GetGameInstance()->GetTimerManager().SetTimer(RestoreTimer,this,&ThisClass::FinishRestoration,Definition->RestoreSeconds,false);
    Changed(NSLOCTEXT("Recovery","Restoring","Recovery program running"));
    return true;
}
float UComputerRecoverySubsystem::GetRestoreProgress() const
{
    if (bRestored) return 1;
    if (!bRestoring || !Definition || !GetGameInstance()) return 0;
    return FMath::Clamp(GetGameInstance()->GetTimerManager().GetTimerElapsed(RestoreTimer)/Definition->RestoreSeconds,0.f,1.f);
}
void UComputerRecoverySubsystem::FinishRestoration()
{
    if (!bRestoring || bRestored) return;
    bRestoring=false; bRestored=true;
    Changed(NSLOCTEXT("Recovery","Online","Main computer restored"));
    OnComputerRestored.Broadcast();
}
void UComputerRecoverySubsystem::ShowObjective(APlayerController* PC)
{
    if (!Definition || !PC || !PC->IsLocalController()) return;
    if (Objective && Objective->GetOwningPlayer()==PC) return;
    if (Objective) Objective->RemoveFromParent();
    Objective=CreateWidget<URecoveryObjectiveWidget>(PC,URecoveryObjectiveWidget::StaticClass());
    if (Objective) Objective->AddToPlayerScreen(5);
}
FText UComputerRecoverySubsystem::GetNextStep() const
{
    if (bRestored) return NSLOCTEXT("Recovery","CompleteHint","All systems restored. The computer is ready.");
    if (bRestoring) return NSLOCTEXT("Recovery","WorkingHint","Recovery continues if you close this terminal.");
    if (GetLoadableCardCount()>0) return NSLOCTEXT("Recovery","LoadHint","Punch cards are ready. Load them into the reader.");
    if (!IsSystemReady(EComputerSystem::Power)) return NSLOCTEXT("Recovery","PowerHint","Connect the wires to restore power.");
    if (!IsSystemReady(EComputerSystem::Fuel)) return NSLOCTEXT("Recovery","FuelHint","Turn the valves to restore the fuel supply.");
    if (!IsSystemReady(EComputerSystem::Display)) return NSLOCTEXT("Recovery","DisplayHint","Align the disks to calibrate the display.");
    if (!IsSystemReady(EComputerSystem::Arithmetic)) return NSLOCTEXT("Recovery","ArithmeticHint","Complete the arithmetic test at the small computer.");
    if (LoadedCards.Num()<RequiredCards.Num()) return NSLOCTEXT("Recovery","FindHint","Find the remaining punch cards for the recovery program.");
    return NSLOCTEXT("Recovery","ReadyHint","All requirements met. Run the recovery program.");
}
void UComputerRecoverySubsystem::Deinitialize()
{
    if (GetGameInstance()) GetGameInstance()->GetTimerManager().ClearTimer(RestoreTimer);
    if (Objective) Objective->RemoveFromParent();
    Objective=nullptr;
    Super::Deinitialize();
}
