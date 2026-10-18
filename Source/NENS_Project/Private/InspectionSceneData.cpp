#include "InspectionSceneData.h"

#include "Engine/Texture2D.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
    bool IsFiniteVector(const FVector2D& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
    }
}

bool UInspectionSceneData::ValidateScene(FString& OutError) const
{
    OutError.Reset();
    if (!IsValid(Background))
    {
        OutError = TEXT("The inspection background texture is missing.");
        return false;
    }
    if (!IsFiniteVector(CanvasSize) || CanvasSize.X <= 0.0 || CanvasSize.Y <= 0.0)
    {
        OutError = TEXT("CanvasSize must contain finite, positive dimensions.");
        return false;
    }

    TSet<FName> PickupIDs;
    for (int32 Index = 0; Index < Pickups.Num(); ++Index)
    {
        const FInspectionPickup& Pickup = Pickups[Index];
        const FString Prefix = FString::Printf(TEXT("Pickup %d (%s): "), Index, *Pickup.PickupID.ToString());
        if (Pickup.PickupID.IsNone() || Pickup.ItemID.IsNone())
        {
            OutError = Prefix + TEXT("PickupID and ItemID must be set.");
            return false;
        }
        if (PickupIDs.Contains(Pickup.PickupID))
        {
            OutError = Prefix + TEXT("PickupID must be unique within this scene.");
            return false;
        }
        PickupIDs.Add(Pickup.PickupID);
        if (!IsValid(Pickup.Image))
        {
            OutError = Prefix + TEXT("The pickup texture is missing.");
            return false;
        }
        if (!IsFiniteVector(Pickup.Size) || Pickup.Size.X <= 0.0 || Pickup.Size.Y <= 0.0)
        {
            OutError = Prefix + TEXT("Size must contain finite, positive dimensions.");
            return false;
        }
        if (!IsFiniteVector(Pickup.Position) || Pickup.Position.X < 0.0 || Pickup.Position.Y < 0.0
            || Pickup.Size.X > CanvasSize.X || Pickup.Size.Y > CanvasSize.Y
            || Pickup.Position.X > CanvasSize.X - Pickup.Size.X
            || Pickup.Position.Y > CanvasSize.Y - Pickup.Size.Y)
        {
            OutError = Prefix + TEXT("The pickup rectangle must fit inside CanvasSize.");
            return false;
        }
    }
    return true;
}

#if WITH_EDITOR
EDataValidationResult UInspectionSceneData::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    FString Error;
    if (!ValidateScene(Error))
    {
        Context.AddError(FText::FromString(Error));
        return EDataValidationResult::Invalid;
    }
    return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif
