#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InspectionSubsystem.h"
#include "InspectionSceneWidget.h"
#include "SearchableComponent.h"

namespace
{
template<class T> T* NamedComponent(AActor* Owner, FName Name)
{
    TInlineComponentArray<T*> Components(Owner);
    for (auto* C : Components) if (C->GetFName() == Name) return C;
    return nullptr;
}
class FInteractionGeometryFlow : public IAutomationLatentCommand
{
public:
    explicit FInteractionGeometryFlow(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 45) { Test->AddError(TEXT("Interaction geometry timed out")); return true; }
        if (Now < Next || !GEditor || !GEditor->PlayWorld) return false;
        UWorld* World = GEditor->PlayWorld;
        auto* PC = World->GetFirstPlayerController();
        if (!PC || !PC->GetLocalPlayer() || !PC->GetPawn()) return false;
        auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
        if (auto* UI = Session->GetActiveWidget()) if (UI->IsOpening() || UI->IsClosing()) return false;
        if (Step == 0)
        {
            auto* Class = LoadClass<AActor>(nullptr, TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_Furniture_Gosling.BP_Furniture_Gosling_C"));
            if (!Test->TestNotNull(TEXT("Picture class"), Class)) return true;
            // Isolate range checks from the room's walls and other nearby furniture.
            Picture = World->SpawnActor<AActor>(Class, FVector(5000,5000,20), FRotator::ZeroRotator);
            if (!Test->TestNotNull(TEXT("Independent picture range fixture"), Picture.Get())) return true;
            Source = Picture->FindComponentByClass<USearchableComponent>();
            auto* Sphere = NamedComponent<USphereComponent>(Picture.Get(), TEXT("Sphere_Interaction"));
            auto* Ring = NamedComponent<UStaticMeshComponent>(Picture.Get(), TEXT("Ring_Visual"));
            auto* Point = NamedComponent<USceneComponent>(Picture.Get(), TEXT("InteractionPoint"));
            auto* Awareness = NamedComponent<USphereComponent>(Picture.Get(), TEXT("Sphere_Awareness"));
            if (!Test->TestNotNull(TEXT("Sphere"), Sphere) || !Test->TestNotNull(TEXT("Ring"), Ring) || !Test->TestNotNull(TEXT("Point"), Point) || !Test->TestNotNull(TEXT("Awareness"), Awareness)) return true;
            Center = Sphere->GetComponentLocation(); Radius = Sphere->GetScaledSphereRadius();
            Test->TestTrue(TEXT("Fixture reproduces offset between old point and interaction center"), FVector::DistXY(Center, Point->GetComponentLocation()) > Radius);
            Test->TestTrue(TEXT("Ring and interaction area share their XY center"), FVector::DistXY(Center, Ring->GetComponentLocation()) < .01);
            Test->TestTrue(TEXT("Gosling ring lies above floor Z=0 and below opaque walls Z=2"), Ring->GetComponentLocation().Z > 0 && Ring->GetComponentLocation().Z < 2);
            Test->TestTrue(TEXT("Awareness contains the whole interaction area"), FVector::DistXY(Center, Awareness->GetComponentLocation()) + Radius <= Awareness->GetScaledSphereRadius());
            if (UStaticMesh* Mesh = Ring->GetStaticMesh())
                Test->TestTrue(TEXT("Visible ring radius matches the interaction radius"), FMath::IsNearlyEqual(Mesh->GetBounds().BoxExtent.X * Ring->GetComponentScale().X, Radius, .01));
            for (auto* C : PC->GetPawn()->GetComponents()) if (C->FindFunction(TEXT("TryInteract"))) Interaction = C;
            if (!Test->TestNotNull(TEXT("Existing interaction input component"), Interaction.Get())) return true;
            Step = 1;
        }
        if (Step == 1)
        {
            if (PositionIndex == 6) return true;
            const FVector Offsets[] = {FVector(-.9 * Radius,0,0), FVector(.9 * Radius,0,0), FVector(0,.9 * Radius,0), FVector(0,-.9 * Radius,0), FVector(Radius+5,0,0), FVector(-Radius-5,0,0)};
            FVector Position = Center + Offsets[PositionIndex]; Position.Z = PC->GetPawn()->GetActorLocation().Z;
            PC->GetPawn()->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
            Step = 2; Next = Now + .3; return false;
        }
        if (Step == 2)
        {
            Interaction->ProcessEvent(Interaction->FindFunction(TEXT("TryInteract")), nullptr);
            if (PositionIndex < 4)
            {
                if (!Test->TestTrue(TEXT("E route opens from every side inside the displaced ring"), Session->IsOpen())) return true;
                Test->TestEqual(TEXT("Correct picture opened"), Session->GetActiveSource(), Source.Get());
                Step = 3;
            }
            else
            {
                Test->TestFalse(TEXT("Outside actual sphere stays unavailable, even near the old point"), Session->IsOpen());
                PositionIndex++; Step = 1;
            }
            Next = Now + .05; return false;
        }
        if (Step == 3) { Session->Close(); Step = 4; return false; }
        Test->TestFalse(TEXT("Range checks restore movement between visits"), PC->IsMoveInputIgnored());
        PositionIndex++; Step = 1; return false;
    }
private:
    FAutomationTestBase* Test;
    double Started, Next = 0;
    int32 Step = 0, PositionIndex = 0;
    FVector Center;
    float Radius = 0;
    TWeakObjectPtr<AActor> Picture;
    TWeakObjectPtr<USearchableComponent> Source;
    TWeakObjectPtr<UActorComponent> Interaction;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionGeometryTest, "NENS.Interaction.Geometry.OffsetSphere", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInteractionGeometryTest::RunTest(const FString&)
{
    FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/NENS/Maps/Tests/Walls/L_WallTest"));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FInteractionGeometryFlow>(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
