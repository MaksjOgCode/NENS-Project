#include "SearchableComponent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"
#include "ComputerRecoverySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSearchable, Log, All);

USearchableComponent::USearchableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool USearchableComponent::IsComplete() const
{
    if (!IsValid(Scene) || Scene->Pickups.IsEmpty())
    {
        return false;
    }
    FString Error;
    if (!Scene->ValidateScene(Error))
    {
        return false;
    }
    for (const FInspectionPickup& Pickup : Scene->Pickups)
    {
        if (!IsCollected(Pickup.PickupID))
        {
            return false;
        }
    }
    return true;
}

bool USearchableComponent::IsCollected(FName PickupID) const
{
    if (PickupID.IsNone()) return false;
    if (CollectedPickupIDs.Contains(PickupID)) return true;
    if (auto* Progress=UComputerRecoverySubsystem::From(this)) return Progress->HasCollected(UComputerRecoverySubsystem::PlacementKey(this,PickupID));
    return false;
}

bool USearchableComponent::TryCollect(FName PickupID)
{
    if (!IsValid(Scene) || PickupID.IsNone() || IsCollected(PickupID))
    {
        return false;
    }
    FString Error;
    if (!Scene->ValidateScene(Error))
    {
        UE_LOG(LogSearchable, Warning, TEXT("Rejected collection for %s: invalid inspection data: %s"),
            *GetPathName(), *Error);
        return false;
    }
    const FInspectionPickup* Pickup = Scene->Pickups.FindByPredicate(
        [PickupID](const FInspectionPickup& Candidate) { return Candidate.PickupID == PickupID; });
    if (!Pickup)
    {
        return false;
    }

    // Copy before broadcasting: a subscriber may change the scene or close the UI.
    const FName ItemID = Pickup->ItemID;
    CollectedPickupIDs.Add(PickupID);
    if (auto* Progress=UComputerRecoverySubsystem::From(this)) Progress->CollectItem(UComputerRecoverySubsystem::PlacementKey(this,PickupID),PickupID,ItemID);
    OnItemCollected.Broadcast(PickupID, ItemID);
    return true;
}

void USearchableComponent::OpenInspection(AActor* Interactor)
{
    if (!IsValid(Interactor))
    {
        return;
    }
    APlayerController* Controller = Cast<APlayerController>(Interactor);
    if (!Controller)
    {
        if (const APawn* Pawn = Cast<APawn>(Interactor))
        {
            Controller = Cast<APlayerController>(Pawn->GetController());
        }
    }
    if (!Controller || !Controller->IsLocalController())
    {
        return;
    }
    if (ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer())
    {
        if (UInspectionSubsystem* Inspection = LocalPlayer->GetSubsystem<UInspectionSubsystem>())
        {
            Inspection->Open(this);
        }
    }
}

void USearchableComponent::NotifyInspectionClosed()
{
    OnInspectionClosed.Broadcast(IsComplete());
}
