#if WITH_DEV_AUTOMATION_TESTS
#include "ComputerRecoveryData.h"
#include "ComputerRecoverySubsystem.h"
#include "InspectionSceneData.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace RecoveryTests
{
UComputerRecoveryData* Definition()
{
    auto* Data=NewObject<UComputerRecoveryData>();
    auto* Scene=NewObject<UInspectionSceneData>(Data);
    Scene->Background=NewObject<UTexture2D>(Scene);
    for (int32 I=0;I<2;++I)
    {
        FInspectionPickup P; P.PickupID=I==0 ? TEXT("WardrobeCard") : TEXT("ClockCard");
        P.ItemID=TEXT("PunchCard"); P.Image=Scene->Background; P.Position=FVector2D(100+I*200,400);
        Scene->Pickups.Add(P);
    }
    Data->CardScenes.Add(Scene); return Data;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryDataTest,"NENS.Recovery.DefinitionValidation",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRecoveryDataTest::RunTest(const FString&)
{
    auto* Data=RecoveryTests::Definition(); TSet<FName> Cards;
    TestTrue(TEXT("Two real placements form a valid program"),Data->GetRequiredCards(Cards));
    TestEqual(TEXT("All cards required"),Cards.Num(),2);
    UInspectionSceneData* RepeatedScene=Data->CardScenes[0];
    Data->CardScenes.Add(RepeatedScene);
    TestFalse(TEXT("Ambiguous repeated placement IDs rejected across scenes"),Data->GetRequiredCards(Cards));
    Data->CardScenes.Pop(); Data->ProgramItemID=TEXT("Note");
    TestFalse(TEXT("Empty program rejected"),Data->GetRequiredCards(Cards));
    Data->ProgramItemID=TEXT("PunchCard"); Data->RestoreSeconds=std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite duration rejected"),Data->GetRequiredCards(Cards));
    Data->RestoreSeconds=3; Data->CardScenes[0]->Pickups[0].Image=nullptr;
    TestFalse(TEXT("Invalid source scene rejected"),Data->GetRequiredCards(Cards));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryInventoryTest,"NENS.Recovery.InventoryAndRequirements",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRecoveryInventoryTest::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>(); auto* P=NewObject<UComputerRecoverySubsystem>(GI);
    auto* Data=RecoveryTests::Definition();
    TestFalse(TEXT("No definition cannot start"),P->CanRestore());
    TestTrue(TEXT("Can collect before computer is spawned"),P->CollectItem(TEXT("wardrobe|card"),TEXT("WardrobeCard"),TEXT("PunchCard")));
    TestFalse(TEXT("Repeated pickup source rejected"),P->CollectItem(TEXT("wardrobe|card"),TEXT("WardrobeCard"),TEXT("PunchCard")));
    TestFalse(TEXT("Missing pickup source rejected"),P->CollectItem(TEXT(""),TEXT("ClockCard"),TEXT("PunchCard")));
    TestEqual(TEXT("No definition cannot load"),P->LoadProgramCards(),0);
    TestTrue(TEXT("Configure current goal"),P->Configure(Data));
    TestTrue(TEXT("Same goal may be reused"),P->Configure(Data));
    TestFalse(TEXT("Another computer cannot silently change the goal"),P->Configure(RecoveryTests::Definition()));
    TestEqual(TEXT("One unique program card available"),P->GetLoadableCardCount(),1);
    TestEqual(TEXT("Partial card loading allowed"),P->LoadProgramCards(),1);
    TestTrue(TEXT("Consumed pickup stays collected"),P->HasCollected(TEXT("wardrobe|card")));
    TestEqual(TEXT("Loaded card removed from bag"),P->GetInventoryCount(TEXT("PunchCard")),0);
    P->CollectItem(TEXT("wardrobe-copy|card"),TEXT("WardrobeCard"),TEXT("PunchCard"));
    P->CollectItem(TEXT("unrelated|card"),TEXT("OtherCard"),TEXT("PunchCard"));
    P->CollectItem(TEXT("table|note"),TEXT("ClockCard"),TEXT("Note"));
    TestEqual(TEXT("Duplicates and other item types cannot replace missing card"),P->GetLoadableCardCount(),0);
    TestEqual(TEXT("Repeated load is a no-op"),P->LoadProgramCards(),0);
    TestFalse(TEXT("None is not a system"),P->MarkSystemReady(EComputerSystem::None,TEXT("invalid")));
    TestFalse(TEXT("Unknown system rejected"),P->MarkSystemReady(static_cast<EComputerSystem>(255),TEXT("invalid")));
    TestTrue(TEXT("Wires restore power"),P->MarkSystemReady(EComputerSystem::Power,TEXT("wires")));
    TestFalse(TEXT("Same source cannot grant a second system"),P->MarkSystemReady(EComputerSystem::Fuel,TEXT("wires")));
    TestFalse(TEXT("Another copy cannot count power twice"),P->MarkSystemReady(EComputerSystem::Power,TEXT("wires-copy")));
    TestTrue(TEXT("Completion remembered for that copy"),P->HasCompletedSource(TEXT("wires-copy")));
    TestTrue(TEXT("Valves restore fuel"),P->MarkSystemReady(EComputerSystem::Fuel,TEXT("valves")));
    TestTrue(TEXT("Disks calibrate display"),P->MarkSystemReady(EComputerSystem::Display,TEXT("disks")));
    TestFalse(TEXT("All systems alone are insufficient"),P->CanRestore());
    TestTrue(TEXT("Missing card collectible after system repairs"),P->CollectItem(TEXT("clock|card"),TEXT("ClockCard"),TEXT("PunchCard")));
    TestFalse(TEXT("Unloaded card still blocks recovery"),P->CanRestore());
    TestEqual(TEXT("Only missing required card loads"),P->LoadProgramCards(),1);
    TestEqual(TEXT("Two distinct cards loaded"),P->GetLoadedCardCount(),2);
    TestEqual(TEXT("Unrelated note remains in bag"),P->GetInventoryCount(TEXT("Note")),1);
    TestFalse(TEXT("Cards and three systems still require arithmetic"),P->CanRestore());
    TestTrue(TEXT("Arithmetic verifies computations"),P->MarkSystemReady(EComputerSystem::Arithmetic,TEXT("arithmetic")));
    TestFalse(TEXT("Arithmetic cannot be counted twice"),P->MarkSystemReady(EComputerSystem::Arithmetic,TEXT("arithmetic")));
    TestEqual(TEXT("Four distinct systems ready"),P->GetReadySystemCount(),4);
    TestTrue(TEXT("All requirements unlock recovery"),P->CanRestore());
    auto* Fresh=NewObject<UComputerRecoverySubsystem>(NewObject<UGameInstance>());
    Fresh->Configure(Data);
    TestFalse(TEXT("New run has no completed systems"),Fresh->IsSystemReady(EComputerSystem::Power));
    TestEqual(TEXT("New run has empty bag"),Fresh->GetInventoryCount(TEXT("PunchCard")),0);
    TestEqual(TEXT("New run has empty reader"),Fresh->GetLoadedCardCount(),0);
    TestFalse(TEXT("New run cannot restore immediately"),Fresh->CanRestore());
    return true;
}
#endif
