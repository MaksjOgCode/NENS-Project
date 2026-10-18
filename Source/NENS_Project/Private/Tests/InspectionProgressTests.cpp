#if WITH_DEV_AUTOMATION_TESTS

#include "InspectionSceneData.h"
#include "SearchableComponent.h"

#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace InspectionProgressTests
{
    UInspectionSceneData* MakeScene()
    {
        UInspectionSceneData* Scene = NewObject<UInspectionSceneData>();
        UTexture2D* Texture = NewObject<UTexture2D>(Scene);
        Scene->Background = Texture;
        for (int32 Index = 0; Index < 2; ++Index)
        {
            FInspectionPickup Pickup;
            Pickup.PickupID = FName(*FString::Printf(TEXT("Card_%d"), Index));
            // Repeated item types are valid; only placed PickupIDs must differ.
            Pickup.ItemID = TEXT("PunchCard");
            Pickup.Image = Texture;
            Pickup.Position = FVector2D(100.0 + Index * 200.0, 500.0);
            Pickup.Size = FVector2D(160.0, 96.0);
            Scene->Pickups.Add(Pickup);
        }
        return Scene;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectionSceneValidationTest,
    "NENS.Inspection.DataValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInspectionSceneValidationTest::RunTest(const FString& Parameters)
{
    UInspectionSceneData* Scene = InspectionProgressTests::MakeScene();
    FString Error;
    TestTrue(TEXT("Unique placements of the same item type are valid"), Scene->ValidateScene(Error));
    TestTrue(TEXT("Valid data clears validation error"), Error.IsEmpty());

    const FInspectionPickup Original = Scene->Pickups[1];
    Scene->Pickups[1].PickupID = Scene->Pickups[0].PickupID;
    TestFalse(TEXT("Duplicate PickupID is rejected"), Scene->ValidateScene(Error));
    TestFalse(TEXT("Invalid data has a diagnostic"), Error.IsEmpty());
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].PickupID = NAME_None;
    TestFalse(TEXT("Missing PickupID is rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].ItemID = NAME_None;
    TestFalse(TEXT("Missing ItemID is rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].Image = nullptr;
    TestFalse(TEXT("Missing pickup image is rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].Size.X = 0.0;
    TestFalse(TEXT("Zero pickup dimensions are rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].Position.Y = -1.0;
    TestFalse(TEXT("Negative pickup position is rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].Position.X = Scene->CanvasSize.X;
    TestFalse(TEXT("Pickup bounds beyond the canvas are rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->Pickups[1].Position.X = std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("Nonfinite position is rejected"), Scene->ValidateScene(Error));
    Scene->Pickups[1] = Original;
    Scene->CanvasSize.X = std::numeric_limits<double>::infinity();
    TestFalse(TEXT("Nonfinite canvas dimensions are rejected"), Scene->ValidateScene(Error));
    Scene->CanvasSize = FVector2D(812.0, 812.0);
    Scene->Background = nullptr;
    TestFalse(TEXT("Missing background is rejected"), Scene->ValidateScene(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectionProgressIsolationTest,
    "NENS.Inspection.ProgressIsolationAndDuplicates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInspectionProgressIsolationTest::RunTest(const FString& Parameters)
{
    UInspectionSceneData* Scene = InspectionProgressTests::MakeScene();
    USearchableComponent* First = NewObject<USearchableComponent>();
    USearchableComponent* Second = NewObject<USearchableComponent>();
    First->Scene = Scene;
    Second->Scene = Scene;
    TestFalse(TEXT("New object is not complete"), First->IsComplete());
    TestFalse(TEXT("Unknown ID cannot be collected"), First->TryCollect(TEXT("Unknown")));
    TestFalse(TEXT("Unset ID cannot be collected"), First->TryCollect(NAME_None));
    TestTrue(TEXT("First click records the first pickup"), First->TryCollect(TEXT("Card_0")));
    TestTrue(TEXT("Progress is readable immediately"), First->IsCollected(TEXT("Card_0")));
    TestFalse(TEXT("Duplicate collection is rejected"), First->TryCollect(TEXT("Card_0")));
    TestFalse(TEXT("Partial progress is not complete"), First->IsComplete());
    TestFalse(TEXT("Objects sharing scene data have independent progress"), Second->IsCollected(TEXT("Card_0")));
    TestTrue(TEXT("Second pickup of the same type is independently collectible"), First->TryCollect(TEXT("Card_1")));
    TestTrue(TEXT("All placements collected completes the first object"), First->IsComplete());
    TestFalse(TEXT("Completing one object does not complete another"), Second->IsComplete());
    TestTrue(TEXT("Another object can collect its own copy"), Second->TryCollect(TEXT("Card_0")));
    TestEqual(TEXT("Collection does not remove definitions from shared data"), Scene->Pickups.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectionEmptyAndInvalidSceneTest,
    "NENS.Inspection.EmptyAndInvalidScene", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInspectionEmptyAndInvalidSceneTest::RunTest(const FString& Parameters)
{
    USearchableComponent* Component = NewObject<USearchableComponent>();
    TestFalse(TEXT("No scene is never complete"), Component->IsComplete());
    TestFalse(TEXT("No scene cannot collect"), Component->TryCollect(TEXT("Card_0")));
    Component->Scene = InspectionProgressTests::MakeScene();
    Component->Scene->Pickups.Reset();
    FString Error;
    TestTrue(TEXT("An empty inspection layout is valid"), Component->Scene->ValidateScene(Error));
    TestFalse(TEXT("An empty scene is not automatically completed"), Component->IsComplete());
    TestFalse(TEXT("An empty scene has no collectible placements"), Component->TryCollect(TEXT("Card_0")));

    Component->Scene = InspectionProgressTests::MakeScene();
    Component->Scene->Pickups[1].PickupID = Component->Scene->Pickups[0].PickupID;
    AddExpectedMessagePlain(TEXT("invalid inspection data"), ELogVerbosity::Warning);
    TestFalse(TEXT("Invalid configuration cannot record collection"), Component->TryCollect(TEXT("Card_0")));
    TestFalse(TEXT("Rejected collection leaves progress unchanged"), Component->IsCollected(TEXT("Card_0")));
    TestFalse(TEXT("Invalid configuration is not complete"), Component->IsComplete());
    return true;
}

#endif
