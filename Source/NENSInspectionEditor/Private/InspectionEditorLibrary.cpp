#include "InspectionEditorLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ActorComponent.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptBlueprint.h"
#include "GameFramework/Actor.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Variable.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace InspectionEditor
{
using FJson = TSharedPtr<FJsonObject>;

FString Serialize(const FJson& Object)
{
    FString Result;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
    FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
    return Result;
}

FJson ObjectDescription(const UObject* Object)
{
    FJson Result = MakeShared<FJsonObject>();
    if (!Object)
    {
        Result->SetBoolField(TEXT("null"), true);
        return Result;
    }

    Result->SetStringField(TEXT("name"), Object->GetName());
    Result->SetStringField(TEXT("path"), Object->GetPathName());
    Result->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
    FJson Properties = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
    {
        FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated))
        {
            continue;
        }

        FString Value;
        const void* Address = Property->ContainerPtrToValuePtr<void>(Object);
        Property->ExportTextItem_Direct(Value, Address, nullptr,
            const_cast<UObject*>(Object), PPF_None);
        Properties->SetStringField(Property->GetName(), Value);
    }
    Result->SetObjectField(TEXT("properties"), Properties);
    return Result;
}

FJson PinDescription(const UEdGraphPin* Pin)
{
    FJson Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), Pin->PinName.ToString());
    Result->SetStringField(TEXT("id"), Pin->PinId.ToString());
    Result->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
    Result->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
    Result->SetStringField(TEXT("subcategory"), Pin->PinType.PinSubCategory.ToString());
    Result->SetStringField(TEXT("type_object"), GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get()));
    Result->SetNumberField(TEXT("container_type"), static_cast<int32>(Pin->PinType.ContainerType));
    Result->SetBoolField(TEXT("reference"), Pin->PinType.bIsReference);
    Result->SetBoolField(TEXT("hidden"), Pin->bHidden);
    Result->SetBoolField(TEXT("orphaned"), Pin->bOrphanedPin);
    Result->SetStringField(TEXT("default"), Pin->DefaultValue);
    Result->SetStringField(TEXT("default_object"), GetPathNameSafe(Pin->DefaultObject));
    Result->SetStringField(TEXT("default_text"), Pin->DefaultTextValue.ToString());
    TArray<TSharedPtr<FJsonValue>> Links;
    for (const UEdGraphPin* Linked : Pin->LinkedTo)
    {
        FJson Link = MakeShared<FJsonObject>();
        Link->SetStringField(TEXT("node"), Linked->GetOwningNode()->GetName());
        Link->SetStringField(TEXT("node_guid"), Linked->GetOwningNode()->NodeGuid.ToString());
        Link->SetStringField(TEXT("pin"), Linked->PinName.ToString());
        Link->SetStringField(TEXT("pin_id"), Linked->PinId.ToString());
        Links.Add(MakeShared<FJsonValueObject>(Link));
    }
    Result->SetArrayField(TEXT("links"), Links);
    return Result;
}
}

FString UInspectionEditorLibrary::DumpBlueprint(UBlueprint* Blueprint)
{
    using namespace InspectionEditor;
    FJson Result = MakeShared<FJsonObject>();
    if (!Blueprint)
    {
        Result->SetStringField(TEXT("error"), TEXT("Blueprint is null"));
        return InspectionEditor::Serialize(Result);
    }

    Result->SetStringField(TEXT("path"), Blueprint->GetPathName());
    Result->SetStringField(TEXT("parent_class"), GetPathNameSafe(Blueprint->ParentClass));
    Result->SetStringField(TEXT("generated_class"), GetPathNameSafe(Blueprint->GeneratedClass));
    Result->SetNumberField(TEXT("status"), static_cast<int32>(Blueprint->Status));
    if (Blueprint->GeneratedClass)
    {
        Result->SetObjectField(TEXT("defaults"), ObjectDescription(Blueprint->GeneratedClass->GetDefaultObject()));
    }

    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    TArray<TSharedPtr<FJsonValue>> JsonGraphs;
    for (const UEdGraph* Graph : Graphs)
    {
        FJson JsonGraph = MakeShared<FJsonObject>();
        JsonGraph->SetStringField(TEXT("name"), Graph->GetName());
        JsonGraph->SetStringField(TEXT("path"), Graph->GetPathName());
        TArray<TSharedPtr<FJsonValue>> Nodes;
        for (const UEdGraphNode* Node : Graph->Nodes)
        {
            FJson JsonNode = ObjectDescription(Node);
            JsonNode->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
            JsonNode->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            JsonNode->SetBoolField(TEXT("enabled"), Node->IsNodeEnabled());
            JsonNode->SetNumberField(TEXT("x"), Node->NodePosX);
            JsonNode->SetNumberField(TEXT("y"), Node->NodePosY);
            if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
            {
                JsonNode->SetStringField(TEXT("function"), Call->FunctionReference.GetMemberName().ToString());
                JsonNode->SetStringField(TEXT("function_class"), GetPathNameSafe(Call->FunctionReference.GetMemberParentClass()));
            }
            if (const UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
            {
                JsonNode->SetStringField(TEXT("event"), Event->EventReference.GetMemberName().ToString());
                JsonNode->SetStringField(TEXT("custom_event"), Event->CustomFunctionName.ToString());
            }
            if (const UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node))
            {
                JsonNode->SetStringField(TEXT("variable"), Variable->VariableReference.GetMemberName().ToString());
            }
            if (const UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Node))
            {
                JsonNode->SetStringField(TEXT("macro"), GetPathNameSafe(Macro->GetMacroGraph()));
            }
            TArray<TSharedPtr<FJsonValue>> Pins;
            for (const UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin)
                {
                    Pins.Add(MakeShared<FJsonValueObject>(PinDescription(Pin)));
                }
            }
            JsonNode->SetArrayField(TEXT("pins"), Pins);
            Nodes.Add(MakeShared<FJsonValueObject>(JsonNode));
        }
        JsonGraph->SetArrayField(TEXT("nodes"), Nodes);
        JsonGraphs.Add(MakeShared<FJsonValueObject>(JsonGraph));
    }
    Result->SetArrayField(TEXT("graphs"), JsonGraphs);

    TArray<TSharedPtr<FJsonValue>> Components;
    UBlueprintGeneratedClass* ActualClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
    for (UBlueprint* Owner = Blueprint; Owner;)
    {
        if (Owner->SimpleConstructionScript)
        {
            for (const USCS_Node* Node : Owner->SimpleConstructionScript->GetAllNodes())
            {
                UActorComponent* Template = ActualClass ? Node->GetActualComponentTemplate(ActualClass) : Node->ComponentTemplate.Get();
                FJson Component = ObjectDescription(Template);
                Component->SetStringField(TEXT("variable"), Node->GetVariableName().ToString());
                Component->SetStringField(TEXT("declared_in"), Owner->GetPathName());
                TArray<TSharedPtr<FJsonValue>> Children;
                for (const USCS_Node* Child : Node->GetChildNodes())
                {
                    Children.Add(MakeShared<FJsonValueString>(Child->GetVariableName().ToString()));
                }
                Component->SetArrayField(TEXT("children"), Children);
                Components.Add(MakeShared<FJsonValueObject>(Component));
            }
        }
        Owner = Owner->ParentClass ? Cast<UBlueprint>(Owner->ParentClass->ClassGeneratedBy) : nullptr;
    }
    Result->SetArrayField(TEXT("components"), Components);

    if (const UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint))
    {
        TArray<TSharedPtr<FJsonValue>> Widgets;
        if (WidgetBlueprint->WidgetTree)
        {
            Result->SetStringField(TEXT("widget_root"), GetNameSafe(WidgetBlueprint->WidgetTree->RootWidget));
            TArray<UWidget*> AllWidgets;
            WidgetBlueprint->WidgetTree->GetAllWidgets(AllWidgets);
            for (const UWidget* Widget : AllWidgets)
            {
                FJson JsonWidget = ObjectDescription(Widget);
                JsonWidget->SetStringField(TEXT("parent"), GetNameSafe(Widget->GetParent()));
                if (Widget->Slot)
                {
                    JsonWidget->SetObjectField(TEXT("slot"), ObjectDescription(Widget->Slot));
                }
                Widgets.Add(MakeShared<FJsonValueObject>(JsonWidget));
            }
        }
        Result->SetArrayField(TEXT("widgets"), Widgets);
    }
    return InspectionEditor::Serialize(Result);
}

FString UInspectionEditorLibrary::CompileAndSave(UBlueprint* Blueprint, bool bSave)
{
    using namespace InspectionEditor;
    FJson Result = MakeShared<FJsonObject>();
    if (!Blueprint)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Blueprint is null"));
        return InspectionEditor::Serialize(Result);
    }

    FCompilerResultsLog Log;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Log);
    Result->SetStringField(TEXT("path"), Blueprint->GetPathName());
    Result->SetNumberField(TEXT("errors"), Log.NumErrors);
    Result->SetNumberField(TEXT("warnings"), Log.NumWarnings);
    Result->SetNumberField(TEXT("status"), static_cast<int32>(Blueprint->Status));
    TArray<TSharedPtr<FJsonValue>> Messages;
    for (const TSharedRef<FTokenizedMessage>& Message : Log.Messages)
    {
        Messages.Add(MakeShared<FJsonValueString>(Message->ToText().ToString()));
    }
    Result->SetArrayField(TEXT("messages"), Messages);
    const bool bCompiled = Log.NumErrors == 0 && Blueprint->Status != BS_Error;
    bool bSaved = false;
    if (bCompiled && bSave)
    {
        UPackage* Package = Blueprint->GetOutermost();
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, Args);
    }
    Result->SetBoolField(TEXT("compiled"), bCompiled);
    Result->SetBoolField(TEXT("saved"), bSaved);
    Result->SetBoolField(TEXT("success"), bCompiled && (!bSave || bSaved));
    return InspectionEditor::Serialize(Result);
}

FString UInspectionEditorLibrary::DumpLevel(const FString& AssetPath)
{
    using namespace InspectionEditor;
    auto Result = MakeShared<FJsonObject>();
    UWorld* World = LoadObject<UWorld>(nullptr, *AssetPath);
    Result->SetBoolField(TEXT("loaded"), World != nullptr);
    if (World && World->PersistentLevel)
    {
        if (auto* BP = World->PersistentLevel->GetLevelScriptBlueprint(true))
        {
            TSharedPtr<FJsonObject> Script;
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(DumpBlueprint(BP)), Script);
            Result->SetObjectField(TEXT("level_script"), Script);
        }
        TArray<TSharedPtr<FJsonValue>> Actors;
        for (AActor* Actor : World->PersistentLevel->Actors)
            if (Actor) Actors.Add(MakeShared<FJsonValueObject>(ObjectDescription(Actor)));
        Result->SetArrayField(TEXT("actors"), Actors);
    }
    return InspectionEditor::Serialize(Result);
}
