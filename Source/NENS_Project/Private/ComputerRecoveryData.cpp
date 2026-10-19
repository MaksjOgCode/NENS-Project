#include "ComputerRecoveryData.h"
#include "InspectionSceneData.h"
bool UComputerRecoveryData::GetRequiredCards(TSet<FName>& OutCards) const
{
    OutCards.Reset();
    if (ProgramItemID.IsNone() || !FMath::IsFinite(RestoreSeconds) || RestoreSeconds < .2f || RestoreSeconds > 20.f) return false;
    for (UInspectionSceneData* Scene : CardScenes)
    {
        FString Error;
        if (!Scene || !Scene->ValidateScene(Error)) return false;
        for (const auto& Pickup : Scene->Pickups)
            if (Pickup.ItemID == ProgramItemID)
            {
                if (OutCards.Contains(Pickup.PickupID)) return false;
                OutCards.Add(Pickup.PickupID);
            }
    }
    return !OutCards.IsEmpty();
}
