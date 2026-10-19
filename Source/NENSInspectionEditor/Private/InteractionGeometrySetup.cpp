#include "InspectionEditorLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString UInspectionEditorLibrary::FixInteractionGeometry()
{
    auto* Interaction = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Gameplay/Interaction/Components/BPC_Interaction.BPC_Interaction"));
    auto* Base = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Picture = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_Furniture_Gosling.BP_Furniture_Gosling"));
    if (!Interaction || !Base || !Picture) return TEXT("{\"success\":false,\"error\":\"Interaction assets missing\"}");
    TArray<UEdGraph*> Graphs; Interaction->GetAllGraphs(Graphs);
    UEdGraph* TargetGraph = nullptr;
    for (auto* Graph : Graphs) if (Graph->GetFName() == TEXT("UpdateInteractionTarget")) TargetGraph = Graph;
    if (!TargetGraph) return TEXT("{\"success\":false,\"error\":\"Target selection graph missing\"}");
    UK2Node_VariableGet* SphereGet = nullptr;
    UK2Node_VariableGet* PointGet = nullptr;
    UK2Node_CallFunction* Location = nullptr;
    for (UEdGraphNode* Node : TargetGraph->Nodes)
    {
        if (auto* Get = Cast<UK2Node_VariableGet>(Node))
            if (Get->VariableReference.GetMemberName() == TEXT("Sphere_Interaction")) SphereGet = Get;
        if (auto* Call = Cast<UK2Node_CallFunction>(Node))
            if (Call->FunctionReference.GetMemberName() == TEXT("K2_GetComponentLocation"))
                if (auto* Self = Call->FindPin(TEXT("self")))
                    if (Self->LinkedTo.Num() == 1)
                        if (auto* Get = Cast<UK2Node_VariableGet>(Self->LinkedTo[0]->GetOwningNode()))
                            if (Get->VariableReference.GetMemberName() == TEXT("InteractionPoint")) { PointGet = Get; Location = Call; }
    }
    if (!SphereGet) return TEXT("{\"success\":false,\"error\":\"Interaction sphere getter missing\"}");
    auto Compiles = [](const FString& Result)
    {
        TSharedPtr<FJsonObject> JSON;
        return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Result), JSON) && JSON->GetBoolField(TEXT("success"));
    };
    if (Location && PointGet)
    {
        Interaction->Modify(); TargetGraph->Modify(); Location->Modify(); SphereGet->Modify();
        auto* Target = Location->FindPinChecked(TEXT("self"));
        Target->BreakAllPinLinks();
        if (!GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(SphereGet->FindPinChecked(TEXT("Sphere_Interaction")), Target))
            return TEXT("{\"success\":false,\"error\":\"Could not connect sphere center\"}");
        FBlueprintEditorUtils::RemoveNode(Interaction, PointGet, true);
        Location->NodeComment = TEXT("Measure XY distance to Sphere_Interaction, including its local offset.");
        Location->bCommentBubbleVisible = true;
        FBlueprintEditorUtils::MarkBlueprintAsModified(Interaction);
        const FString Result = CompileAndSave(Interaction);
        if (!Compiles(Result)) return Result;
    }
    else
    {
        // Be idempotent, but do not silently accept a different graph topology.
        bool bAlreadyFixed = false;
        for (UEdGraphNode* Node : TargetGraph->Nodes)
            if (auto* Call = Cast<UK2Node_CallFunction>(Node))
                if (Call->FunctionReference.GetMemberName() == TEXT("K2_GetComponentLocation"))
                    if (auto* Self = Call->FindPin(TEXT("self")))
                        bAlreadyFixed |= Self->LinkedTo.Contains(SphereGet->FindPinChecked(TEXT("Sphere_Interaction")));
        if (!bAlreadyFixed) return TEXT("{\"success\":false,\"error\":\"Unexpected distance graph; no assets replaced\"}");
    }

    auto* SphereNode = Base->SimpleConstructionScript->FindSCSNode(TEXT("Sphere_Interaction"));
    auto* RingNode = Base->SimpleConstructionScript->FindSCSNode(TEXT("Ring_Visual"));
    if (!SphereNode || !RingNode) return TEXT("{\"success\":false,\"error\":\"Base ring or sphere missing\"}");
    Picture->Modify();
    auto* Handler = Picture->GetInheritableComponentHandler(true);
    Handler->Modify();
    auto Override = [Handler](USCS_Node* Node)
    {
        const FComponentKey Key(Node);
        auto* Template = Handler->GetOverridenComponentTemplate(Key);
        return Template ? Template : Handler->CreateOverridenComponentTemplate(Key);
    };
    auto* Sphere = Cast<USphereComponent>(Override(SphereNode));
    auto* Ring = Cast<USceneComponent>(Override(RingNode));
    if (!Sphere || !Ring) return TEXT("{\"success\":false,\"error\":\"Picture component override failed\"}");
    Sphere->Modify(); Ring->Modify();
    // The picture's existing 100-unit ring mesh has unit scale: its outer bound is radius 50.
    Sphere->SetSphereRadius(50, false);
    FVector Relative = Ring->GetRelativeLocation();
    // L_WallTest: picture Z=20, floor Z=0, opaque walls Z=2. Keep XY artist placement.
    Relative.Z = -19;
    Ring->SetRelativeLocation(Relative);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Picture);
    return CompileAndSave(Picture);
}
