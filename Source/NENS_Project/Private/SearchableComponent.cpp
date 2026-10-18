#include "SearchableComponent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InspectionSceneData.h"
#include "InspectionSubsystem.h"

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
        if (!CollectedPickupIDs.Contains(Pickup.PickupID))
        {
            return false;
        }
    }
    return true;
}

bool USearchableComponent::IsCollected(FName PickupID) const
{
    return !PickupID.IsNone() && CollectedPickupIDs.Contains(PickupID);
}

bool USearchableComponent::TryCollect(FName PickupID)
{
    if (!IsValid(Scene) || PickupID.IsNone() || CollectedPickupIDs.Contains(PickupID))
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
