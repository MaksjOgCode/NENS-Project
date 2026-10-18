#include "InspectionEditorLibrary.h"
#include "InspectionBlueprintLibrary.h"
#include "InspectionSceneData.h"
#include "InspectionSceneWidget.h"
#include "SearchableComponent.h"
#include "MinigameComponent.h"
#include "MinigameWidget.h"
#include "BlueprintEditorLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/SceneComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/Texture2D.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

namespace InspectionSetup
{
FString SetupError;
bool Link(UEdGraphPin* A, UEdGraphPin* B)
{
    if (!A || !B || !GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A, B))
    {
        SetupError = TEXT("Cannot connect pins: ") + (A ? A->PinName.ToString() : TEXT("null")) + TEXT(" / ") + (B ? B->PinName.ToString() : TEXT("null"));
        return false;
    }
    return true;
}
UEdGraph* Graph(UBlueprint* BP, FName Name)
{
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    for (auto* G : Graphs) if (G->GetFName() == Name) return G;
    return nullptr;
}
UEdGraphNode* Node(UEdGraph* G, FName Name)
{
    if (G) for (UEdGraphNode* N : G->Nodes) if (N->GetFName() == Name) return N;
    return nullptr;
}
bool HasMarker(UEdGraph* G, const FString& Marker)
{
    for (UEdGraphNode* N : G->Nodes) if (N->NodeComment == Marker) return true;
    return false;
}
template<class T> T* Add(UEdGraph* G, int32 X, int32 Y, const FString& Comment = TEXT(""))
{
    FGraphNodeCreator<T> Creator(*G);
    T* N = Creator.CreateNode(); Creator.Finalize();
    N->NodePosX = X; N->NodePosY = Y; N->NodeComment = Comment;
    return N;
}
UK2Node_CallFunction* Call(UEdGraph* G, UClass* Class, FName Name, int32 X, int32 Y, const FString& Comment = TEXT(""))
{
    UFunction* Fn = Class->FindFunctionByName(Name);
    if (!Fn) { SetupError = TEXT("Missing function ") + Name.ToString(); return nullptr; }
    FGraphNodeCreator<UK2Node_CallFunction> Creator(*G);
    auto* N = Creator.CreateNode(); N->SetFromFunction(Fn); Creator.Finalize();
    N->NodePosX = X; N->NodePosY = Y; N->NodeComment = Comment;
    return N;
}
bool InsertBranch(UEdGraphPin* Exec, UEdGraphPin* Condition, int32 X, int32 Y)
{
    TArray<UEdGraphPin*> Previous = Exec->LinkedTo;
    Exec->BreakAllPinLinks();
    auto* B = Add<UK2Node_IfThenElse>(Exec->GetOwningNode()->GetGraph(), X, Y);
    if (!Link(Exec, B->GetExecPin()) || !Link(Condition, B->GetConditionPin())) return false;
    for (auto* P : Previous) if (!Link(B->GetThenPin(), P)) return false;
    return true;
}
bool SuccessJSON(const FString& JSON)
{
    TSharedPtr<FJsonObject> O;
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JSON), O) && O->GetBoolField(TEXT("success"));
}
bool Save(UObject* Object)
{
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Object->GetOutermost(), Object,
        *FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
FString Fail(const FString& Message)
{
    auto O = MakeShared<FJsonObject>(); O->SetBoolField(TEXT("success"), false); O->SetStringField(TEXT("error"), Message);
    FString S; FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&S)); return S;
}
}

#define REQUIRE_SETUP(Condition, Message) if (!(Condition)) return InspectionSetup::Fail(Message)

FString UInspectionEditorLibrary::ConfigureInspection()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* Base = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Interaction = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Gameplay/Interaction/Components/BPC_Interaction.BPC_Interaction"));
    auto* Wardrobe = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_Wardrobe.BP_Wardrobe"));
    auto* Widget = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/NENS/UI/Inspection/WBP_InspectionScene.WBP_InspectionScene"));
    REQUIRE_SETUP(Base && Interaction && Wardrobe && Widget, TEXT("Required Blueprint is missing"));
    auto* Background = LoadObject<UTexture2D>(nullptr, TEXT("/Game/NENS/UI/Inspection/Textures/T_Inspection_Wardrobe.T_Inspection_Wardrobe"));
    auto* Card = LoadObject<UTexture2D>(nullptr, TEXT("/Game/NENS/UI/Inspection/Textures/T_PunchCard.T_PunchCard"));
    REQUIRE_SETUP(Background && Card, TEXT("Existing inspection textures are missing"));

    // Extend the user's existing designer layout; no duplicate screen asset.
    Widget->Modify();
    UBlueprintEditorLibrary::ReparentBlueprint(Widget, UInspectionSceneWidget::StaticClass());
    UWidgetTree* Tree = Widget->WidgetTree;
    auto* Root = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_Root")));
    auto* Scale = Cast<UScaleBox>(Tree->FindWidget(TEXT("ScaleBox_Scene")));
    REQUIRE_SETUP(Root && Scale && Scale->GetParent() == Root, TEXT("Unexpected root/ScaleBox hierarchy"));
    Root->SetVisibility(ESlateVisibility::Visible);
    Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Scale->SetStretch(EStretch::ScaleToFit);
    auto* ScaleSlot = Cast<UCanvasPanelSlot>(Scale->Slot);
    ScaleSlot->SetAnchors(FAnchors(0, 0, 1, 1)); ScaleSlot->SetOffsets(FMargin(0)); ScaleSlot->SetZOrder(1);
    auto* Size = Cast<USizeBox>(Tree->FindWidget(TEXT("SizeBox_Scene")));
    if (!Size) { Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Scene")); Widget->OnVariableAdded(Size->GetFName()); Scale->AddChild(Size); }
    REQUIRE_SETUP(Size->GetParent() == Scale, TEXT("Unexpected SizeBox parent"));
    Size->SetWidthOverride(812); Size->SetHeightOverride(812);
    auto* SS = Cast<UScaleBoxSlot>(Size->Slot); SS->SetHorizontalAlignment(HAlign_Center); SS->SetVerticalAlignment(VAlign_Center);
    auto* Canvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Canvas_Scene")));
    if (!Canvas) { Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_Scene")); Widget->OnVariableAdded(Canvas->GetFName()); Size->AddChild(Canvas); }
    REQUIRE_SETUP(Canvas->GetParent() == Size, TEXT("Unexpected scene Canvas parent"));
    auto* Image = Cast<UImage>(Tree->FindWidget(TEXT("Image_Background")));
    if (!Image) { Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Background")); Widget->OnVariableAdded(Image->GetFName()); Canvas->AddChild(Image); }
    REQUIRE_SETUP(Image->GetParent() == Canvas, TEXT("Unexpected background parent"));
    Image->SetBrushFromTexture(Background); Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* ImageSlot = Cast<UCanvasPanelSlot>(Image->Slot); ImageSlot->SetAnchors(FAnchors(0,0,1,1)); ImageSlot->SetOffsets(FMargin(0));
    auto* Backdrop = Cast<UImage>(Tree->FindWidget(TEXT("Image_Backdrop")));
    if (!Backdrop) { Backdrop = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Backdrop")); Widget->OnVariableAdded(Backdrop->GetFName()); Root->AddChild(Backdrop); }
    Backdrop->SetColorAndOpacity(FLinearColor::Black); Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BackdropSlot = Cast<UCanvasPanelSlot>(Backdrop->Slot); BackdropSlot->SetAnchors(FAnchors(0,0,1,1)); BackdropSlot->SetOffsets(FMargin(0)); BackdropSlot->SetZOrder(0);
    auto* Hint = Cast<UTextBlock>(Tree->FindWidget(TEXT("Text_ExitHint")));
    if (!Hint) { Hint = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_ExitHint")); Widget->OnVariableAdded(Hint->GetFName()); Root->AddChild(Hint); }
    Hint->SetText(FText::FromString(TEXT("Esc — выйти"))); Hint->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Hint->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* HintSlot = Cast<UCanvasPanelSlot>(Hint->Slot); HintSlot->SetAnchors(FAnchors(1,1)); HintSlot->SetAlignment(FVector2D(1,1)); HintSlot->SetPosition(FVector2D(-24,-16)); HintSlot->SetAutoSize(true); HintSlot->SetZOrder(2);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
    REQUIRE_SETUP(SuccessJSON(CompileAndSave(Widget, false)), TEXT("Inspection widget compile failed"));
    CastChecked<UInspectionSceneWidget>(Widget->GeneratedClass->GetDefaultObject())->SetIsFocusable(true);

    const TCHAR* DataPath = TEXT("/Game/NENS/UI/Inspection/DA_Inspection_Wardrobe");
    auto* Data = FPackageName::DoesPackageExist(DataPath) ? LoadObject<UInspectionSceneData>(nullptr, *(FString(DataPath) + TEXT(".DA_Inspection_Wardrobe"))) : nullptr;
    if (!Data)
    {
        Data = NewObject<UInspectionSceneData>(CreatePackage(DataPath), TEXT("DA_Inspection_Wardrobe"), RF_Public | RF_Standalone | RF_Transactional);
        Data->Background = Background;
        FInspectionPickup Pickup; Pickup.PickupID = TEXT("Wardrobe_PunchCard"); Pickup.ItemID = TEXT("PunchCard");
        Pickup.Image = Card; Pickup.Position = FVector2D(316,532); Pickup.Size = FVector2D(180,108);
        Data->Pickups.Add(Pickup);
    }
    FString DataError; REQUIRE_SETUP(Data->ValidateScene(DataError), DataError);

    Wardrobe->Modify();
    USimpleConstructionScript* SCS = Wardrobe->SimpleConstructionScript;
    REQUIRE_SETUP(SCS, TEXT("Wardrobe has no SCS"));
    auto* SearchNode = SCS->FindSCSNode(TEXT("Searchable"));
    if (!SearchNode) { SearchNode = SCS->CreateNode(USearchableComponent::StaticClass(), TEXT("Searchable")); SCS->AddNode(SearchNode); }
    auto* Search = Cast<USearchableComponent>(SearchNode->ComponentTemplate);
    REQUIRE_SETUP(Search, TEXT("Searchable name is occupied by a different component"));
    Search->Scene = Data; Search->SceneWidgetClass = Widget->GeneratedClass;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Wardrobe);
    REQUIRE_SETUP(SuccessJSON(CompileAndSave(Wardrobe, false)), TEXT("Wardrobe component compile failed"));

    auto* WG = Graph(Wardrobe, TEXT("EventGraph"));
    REQUIRE_SETUP(WG, TEXT("Wardrobe EventGraph missing"));
    if (!HasMarker(WG, TEXT("Inspection.Open")))
    {
        UK2Node_Event* Event = nullptr;
        for (UEdGraphNode* N : WG->Nodes) if (auto* E = Cast<UK2Node_Event>(N)) if (E->EventReference.GetMemberName() == TEXT("Interact")) Event = E;
        REQUIRE_SETUP(Event, TEXT("Wardrobe Interact event missing"));
        // Verify we only replace the old demonstration print.
        for (auto* P : Event->FindPinChecked(TEXT("then"))->LinkedTo)
        {
            auto* Old = Cast<UK2Node_CallFunction>(P->GetOwningNode());
            REQUIRE_SETUP(Old && Old->FunctionReference.GetMemberName() == TEXT("PrintString"), TEXT("Interact has non-demo logic; refusing to replace"));
        }
        auto* Open = Call(WG, UInspectionBlueprintLibrary::StaticClass(), TEXT("OpenInspection"), 650, 1200, TEXT("Inspection.Open"));
        auto* Self = Add<UK2Node_Self>(WG, 400, 1400);
        Event->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
        REQUIRE_SETUP(Link(Event->FindPinChecked(TEXT("then")), Open->GetExecPin()) && Link(Event->FindPinChecked(TEXT("Interactor")), Open->FindPinChecked(TEXT("Interactor"))) && Link(Self->FindPinChecked(TEXT("self")), Open->FindPinChecked(TEXT("Target"))), SetupError);
    }
    if (!HasMarker(WG, TEXT("Inspection.Closed")))
    {
        auto* Property = FindFProperty<FObjectProperty>(Wardrobe->GeneratedClass, TEXT("Searchable"));
        auto* Delegate = FindFProperty<FMulticastDelegateProperty>(USearchableComponent::StaticClass(), TEXT("OnInspectionClosed"));
        REQUIRE_SETUP(Property && Delegate, TEXT("Component event properties missing"));
        FGraphNodeCreator<UK2Node_ComponentBoundEvent> Creator(*WG);
        auto* Event = Creator.CreateNode(); Event->InitializeComponentBoundEventParams(Property, Delegate); Creator.Finalize();
        Event->NodePosX = 200; Event->NodePosY = 1700; Event->NodeComment = TEXT("Inspection.Closed");
        auto* Branch = Add<UK2Node_IfThenElse>(WG, 650, 1700);
        auto* Hide = Call(WG, Base->GeneratedClass, TEXT("SetAwarenessVisible"), 950, 1700);
        Hide->FindPinChecked(TEXT("IsVisible"))->DefaultValue = TEXT("false");
        REQUIRE_SETUP(Link(Event->FindPinChecked(TEXT("then")), Branch->GetExecPin()) && Link(Event->FindPinChecked(TEXT("bCompleted")), Branch->GetConditionPin()) && Link(Branch->GetThenPin(), Hide->GetExecPin()), SetupError);
    }

    // Gate presentation even if a completed owner later receives another overlap.
    auto* BG = Graph(Base, TEXT("SetAwarenessVisible"));
    REQUIRE_SETUP(BG, TEXT("Base awareness function missing"));
    if (!HasMarker(BG, TEXT("Inspection.Availability")))
    {
        auto* Entry = Node(BG, TEXT("K2Node_FunctionEntry_0"));
        REQUIRE_SETUP(Entry && Entry->FindPin(TEXT("IsVisible")), TEXT("Awareness input missing"));
        auto* Input = Entry->FindPinChecked(TEXT("IsVisible"));
        TArray<UEdGraphPin*> OldLinks = Input->LinkedTo; Input->BreakAllPinLinks();
        auto* Available = Call(BG, UInspectionBlueprintLibrary::StaticClass(), TEXT("IsInteractionAvailable"), -200, 300, TEXT("Inspection.Availability"));
        auto* Self = Add<UK2Node_Self>(BG, -450, 450);
        auto* And = Call(BG, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 100, 300);
        REQUIRE_SETUP(Link(Self->FindPinChecked(TEXT("self")), Available->FindPinChecked(TEXT("Target"))) && Link(Input, And->FindPinChecked(TEXT("A"))) && Link(Available->GetReturnValuePin(), And->FindPinChecked(TEXT("B"))), SetupError);
        for (auto* Pin : OldLinks) REQUIRE_SETUP(Link(And->GetReturnValuePin(), Pin), SetupError);
    }
    auto* UG = Graph(Interaction, TEXT("UpdateInteractionTarget"));
    REQUIRE_SETUP(UG, TEXT("Target selection function missing"));
    if (!HasMarker(UG, TEXT("Inspection.Availability")))
    {
        auto* Valid = Node(UG, TEXT("K2Node_MacroInstance_1"));
        REQUIRE_SETUP(Valid && Valid->FindPin(TEXT("InputObject")) && Valid->FindPin(TEXT("Is Valid")), TEXT("Candidate validity check changed"));
        auto* Object = Valid->FindPinChecked(TEXT("InputObject"));
        REQUIRE_SETUP(Object->LinkedTo.Num() == 1, TEXT("Candidate source is ambiguous"));
        auto* Available = Call(UG, UInspectionBlueprintLibrary::StaticClass(), TEXT("IsInteractionAvailable"), Valid->NodePosX, Valid->NodePosY + 300, TEXT("Inspection.Availability"));
        REQUIRE_SETUP(Link(Object->LinkedTo[0], Available->FindPinChecked(TEXT("Target"))) && InsertBranch(Valid->FindPinChecked(TEXT("Is Valid")), Available->GetReturnValuePin(), Valid->NodePosX+400, Valid->NodePosY), SetupError);
    }
    auto* TG = Graph(Interaction, TEXT("TryInteract"));
    REQUIRE_SETUP(TG, TEXT("TryInteract function missing"));
    if (!HasMarker(TG, TEXT("Inspection.Availability")))
    {
        auto* Valid = Node(TG, TEXT("K2Node_MacroInstance_0"));
        auto* Owner = Node(TG, TEXT("K2Node_CallFunction_2"));
        REQUIRE_SETUP(Valid && Owner && Valid->FindPinChecked(TEXT("InputObject"))->LinkedTo.Num() == 1, TEXT("TryInteract topology changed"));
        auto* Available = Call(TG, UInspectionBlueprintLibrary::StaticClass(), TEXT("IsInteractionAvailable"), 800, 500, TEXT("Inspection.Availability"));
        auto* Allowed = Call(TG, UInspectionBlueprintLibrary::StaticClass(), TEXT("IsInteractionInputAllowed"), 800, 700);
        auto* And = Call(TG, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 1150, 600);
        REQUIRE_SETUP(Link(Valid->FindPinChecked(TEXT("InputObject"))->LinkedTo[0], Available->FindPinChecked(TEXT("Target"))) && Link(Owner->FindPinChecked(TEXT("ReturnValue")), Allowed->FindPinChecked(TEXT("Interactor"))) && Link(Available->GetReturnValuePin(), And->FindPinChecked(TEXT("A"))) && Link(Allowed->GetReturnValuePin(), And->FindPinChecked(TEXT("B"))) && InsertBranch(Valid->FindPinChecked(TEXT("Is Valid")), And->GetReturnValuePin(), 1400, 200), SetupError);
    }
    TArray<UBlueprint*> Blueprints = {Base, Interaction, Widget, Wardrobe};
    for (auto* BP : Blueprints)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FString Result = CompileAndSave(BP, false);
        REQUIRE_SETUP(SuccessJSON(Result), Result);
    }
    CastChecked<UInspectionSceneWidget>(Widget->GeneratedClass->GetDefaultObject())->SetIsFocusable(true);
    for (auto* BP : Blueprints) REQUIRE_SETUP(Save(BP), TEXT("Save failed: ") + BP->GetPathName());
    REQUIRE_SETUP(Save(Data), TEXT("Scene data save failed"));
    const FString PromptFix = FixInspectionPrompt();
    REQUIRE_SETUP(SuccessJSON(PromptFix), PromptFix);
    return TEXT("{\"success\":true,\"blueprints\":5,\"scene\":\"DA_Inspection_Wardrobe\"}");
}

FString UInspectionEditorLibrary::FixInspectionPrompt()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* Prompt = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/NENS/UI/Interaction/WBP_InteractionPrompt.WBP_InteractionPrompt"));
    auto* Wardrobe = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_Wardrobe.BP_Wardrobe"));
    REQUIRE_SETUP(Prompt && Wardrobe, TEXT("Prompt or wardrobe Blueprint is missing"));
    auto* PG = Graph(Prompt, TEXT("SetPromptFocused"));
    auto* WG = Graph(Wardrobe, TEXT("EventGraph"));
    REQUIRE_SETUP(PG && WG, TEXT("Expected prompt/wardrobe graph is missing"));

    if (!HasMarker(PG, TEXT("Inspection.PromptStateGuard")))
    {
        auto* Entry = Node(PG, TEXT("K2Node_FunctionEntry_0"));
        REQUIRE_SETUP(Entry && Entry->FindPin(TEXT("IsFocused")), TEXT("Prompt focus input is missing"));
        const FName StateName(TEXT("bPromptFocused"));
        FEdGraphPinType BoolType;
        BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        REQUIRE_SETUP(FBlueprintEditorUtils::AddMemberVariable(Prompt, StateName, BoolType, TEXT("false")), TEXT("Could not add prompt state"));

        FGraphNodeCreator<UK2Node_VariableGet> GetCreator(*PG);
        auto* GetState = GetCreator.CreateNode(); GetState->VariableReference.SetSelfMember(StateName); GetCreator.Finalize();
        GetState->NodePosX = -700; GetState->NodePosY = 300;
        FGraphNodeCreator<UK2Node_VariableSet> SetCreator(*PG);
        auto* SetState = SetCreator.CreateNode(); SetState->VariableReference.SetSelfMember(StateName); SetCreator.Finalize();
        SetState->NodePosX = -250; SetState->NodePosY = 0;
        auto* Changed = Call(PG, UKismetMathLibrary::StaticClass(), TEXT("NotEqual_BoolBool"), -450, 300, TEXT("Inspection.PromptStateGuard"));
        auto* Guard = Add<UK2Node_IfThenElse>(PG, -450, 0);
        auto* Exec = Entry->FindPinChecked(TEXT("then"));
        TArray<UEdGraphPin*> Previous = Exec->LinkedTo;
        REQUIRE_SETUP(Previous.Num() == 1, TEXT("Prompt execution topology changed"));
        Exec->BreakAllPinLinks();
        REQUIRE_SETUP(Link(Entry->FindPinChecked(TEXT("IsFocused")), Changed->FindPinChecked(TEXT("A")))
            && Link(GetState->FindPinChecked(StateName), Changed->FindPinChecked(TEXT("B")))
            && Link(Exec, Guard->GetExecPin())
            && Link(Changed->GetReturnValuePin(), Guard->GetConditionPin())
            && Link(Guard->GetThenPin(), SetState->GetExecPin())
            && Link(Entry->FindPinChecked(TEXT("IsFocused")), SetState->FindPinChecked(StateName))
            && Link(SetState->GetThenPin(), Previous[0]), SetupError);
    }

    if (!HasMarker(WG, TEXT("Inspection.RetireIndicators")))
    {
        UEdGraphNode* Closed = nullptr;
        for (UEdGraphNode* N : WG->Nodes) if (N->NodeComment == TEXT("Inspection.Closed")) Closed = N;
        REQUIRE_SETUP(Closed && Closed->FindPinChecked(TEXT("then"))->LinkedTo.Num() == 1, TEXT("Inspection closed event changed"));
        auto* Branch = Cast<UK2Node_IfThenElse>(Closed->FindPinChecked(TEXT("then"))->LinkedTo[0]->GetOwningNode());
        REQUIRE_SETUP(Branch && Branch->GetThenPin()->LinkedTo.Num() == 1, TEXT("Completion branch changed"));
        auto* HideAwareness = Cast<UK2Node_CallFunction>(Branch->GetThenPin()->LinkedTo[0]->GetOwningNode());
        REQUIRE_SETUP(HideAwareness && HideAwareness->FunctionReference.GetMemberName() == TEXT("SetAwarenessVisible"), TEXT("Completion hide call changed"));
        auto* Exec = HideAwareness->GetThenPin();
        TArray<UEdGraphPin*> Previous = Exec->LinkedTo;
        Exec->BreakAllPinLinks();
        int32 X = HideAwareness->NodePosX + 400;
        for (FName Name : {FName(TEXT("Widget_InteractionPrompt")), FName(TEXT("Ring_Visual"))})
        {
            REQUIRE_SETUP(FindFProperty<FObjectProperty>(Wardrobe->GeneratedClass, Name), TEXT("Indicator property missing: ") + Name.ToString());
            FGraphNodeCreator<UK2Node_VariableGet> Creator(*WG);
            auto* Get = Creator.CreateNode(); Get->VariableReference.SetSelfMember(Name); Creator.Finalize();
            Get->NodePosX = X; Get->NodePosY = 1950;
            auto* Hide = Call(WG, USceneComponent::StaticClass(), TEXT("SetHiddenInGame"), X, 1700,
                Name == TEXT("Widget_InteractionPrompt") ? TEXT("Inspection.RetireIndicators") : TEXT(""));
            REQUIRE_SETUP(Hide && Hide->FindPin(TEXT("NewHidden")), TEXT("SetHiddenInGame signature changed"));
            Hide->FindPinChecked(TEXT("NewHidden"))->DefaultValue = TEXT("true");
            REQUIRE_SETUP(Link(Exec, Hide->GetExecPin()) && Link(Get->FindPinChecked(Name), Hide->FindPinChecked(TEXT("self"))), SetupError);
            Exec = Hide->GetThenPin(); X += 400;
        }
        for (auto* Pin : Previous) REQUIRE_SETUP(Link(Exec, Pin), SetupError);
    }

    for (UBlueprint* BP : {static_cast<UBlueprint*>(Prompt), Wardrobe})
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        const FString Result = CompileAndSave(BP, false);
        REQUIRE_SETUP(SuccessJSON(Result), Result);
    }
    REQUIRE_SETUP(Save(Prompt) && Save(Wardrobe), TEXT("Could not save prompt fix"));
    return TEXT("{\"success\":true,\"blueprints\":2,\"fix\":\"prompt-state-and-completion-visibility\"}");
}
FString UInspectionEditorLibrary::ConfigureWiresMinigame()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* Widget = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Minigames/Wires/WBP_WiresMiniGame.WBP_WiresMiniGame"));
    auto* Computer = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_ComputerCabinet.BP_ComputerCabinet"));
    auto* Base = LoadObject<UBlueprint>(nullptr, TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    REQUIRE_SETUP(Widget && Computer && Base, TEXT("Wires widget, computer or interaction base is missing"));
    Widget->Modify();
    if (Widget->ParentClass != UMinigameWidget::StaticClass()) UBlueprintEditorLibrary::ReparentBlueprint(Widget, UMinigameWidget::StaticClass());
    auto* CloseGraph = Graph(Widget, TEXT("CloseMiniGame"));
    auto* EventGraph = Graph(Widget, TEXT("EventGraph"));
    REQUIRE_SETUP(CloseGraph && EventGraph, TEXT("Wires CloseMiniGame/EventGraph missing"));
    if (!HasMarker(CloseGraph, TEXT("Minigame.SessionClose")))
    {
        auto* Entry = Node(CloseGraph, TEXT("K2Node_FunctionEntry_0"));
        REQUIRE_SETUP(Entry, TEXT("CloseMiniGame entry missing"));
        auto* Request = Call(CloseGraph, UMinigameWidget::StaticClass(), TEXT("RequestClose"), 0, -300, TEXT("Minigame.SessionClose"));
        Entry->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
        REQUIRE_SETUP(Link(Entry->FindPinChecked(TEXT("then")), Request->GetExecPin()), SetupError);
    }
    if (!HasMarker(EventGraph, TEXT("Minigame.ReportSuccess")))
    {
        UK2Node_CustomEvent* Win = nullptr;
        for (UEdGraphNode* N : EventGraph->Nodes)
            if (auto* E = Cast<UK2Node_CustomEvent>(N)) if (E->CustomFunctionName == TEXT("Win")) Win = E;
        REQUIRE_SETUP(Win && Win->FindPinChecked(TEXT("then"))->LinkedTo.Num() == 1, TEXT("Wires Win event changed"));
        auto* Status = Cast<UK2Node_CallFunction>(Win->FindPinChecked(TEXT("then"))->LinkedTo[0]->GetOwningNode());
        REQUIRE_SETUP(Status && Status->FunctionReference.GetMemberName() == TEXT("SetText"), TEXT("Wires win status topology changed"));
        auto* Exec = Status->GetThenPin();
        TArray<UEdGraphPin*> Next = Exec->LinkedTo;
        auto* Report = Call(EventGraph, UMinigameWidget::StaticClass(), TEXT("ReportSuccess"), Status->NodePosX + 300, Status->NodePosY + 200, TEXT("Minigame.ReportSuccess"));
        Exec->BreakAllPinLinks();
        REQUIRE_SETUP(Link(Exec, Report->GetExecPin()), SetupError);
        for (auto* P : Next) REQUIRE_SETUP(Link(Report->GetThenPin(), P), SetupError);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
    REQUIRE_SETUP(SuccessJSON(CompileAndSave(Widget, false)), TEXT("Adapted wires widget compile failed"));
    CastChecked<UMinigameWidget>(Widget->GeneratedClass->GetDefaultObject())->SetIsFocusable(true);

    Computer->Modify();
    USimpleConstructionScript* SCS = Computer->SimpleConstructionScript;
    REQUIRE_SETUP(SCS, TEXT("Computer SCS missing"));
    auto* ComponentNode = SCS->FindSCSNode(TEXT("Minigame"));
    if (!ComponentNode) { ComponentNode = SCS->CreateNode(UMinigameComponent::StaticClass(), TEXT("Minigame")); SCS->AddNode(ComponentNode); }
    auto* Component = Cast<UMinigameComponent>(ComponentNode->ComponentTemplate);
    REQUIRE_SETUP(Component, TEXT("Minigame name belongs to another component"));
    Component->WidgetClass = Widget->GeneratedClass;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Computer);
    REQUIRE_SETUP(SuccessJSON(CompileAndSave(Computer, false)), TEXT("Computer component compile failed"));
    auto* CG = Graph(Computer, TEXT("EventGraph"));
    REQUIRE_SETUP(CG, TEXT("Computer EventGraph missing"));
    if (!HasMarker(CG, TEXT("Minigame.Open")))
    {
        UK2Node_Event* Interact = nullptr;
        for (UEdGraphNode* N : CG->Nodes)
            if (auto* E = Cast<UK2Node_Event>(N)) if (E->EventReference.GetMemberName() == TEXT("Interact")) Interact = E;
        REQUIRE_SETUP(Interact, TEXT("Computer Interact event missing"));
        for (auto* P : Interact->FindPinChecked(TEXT("then"))->LinkedTo)
        {
            auto* Old = Cast<UK2Node_CallFunction>(P->GetOwningNode());
            REQUIRE_SETUP(Old && Old->FunctionReference.GetMemberName() == TEXT("PrintString"), TEXT("Computer has non-demo interaction logic; refusing to replace"));
        }
        auto* Open = Call(CG, UInspectionBlueprintLibrary::StaticClass(), TEXT("OpenMinigame"), 650, 1200, TEXT("Minigame.Open"));
        auto* Self = Add<UK2Node_Self>(CG, 400, 1400);
        Interact->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
        REQUIRE_SETUP(Link(Interact->FindPinChecked(TEXT("then")), Open->GetExecPin())
            && Link(Interact->FindPinChecked(TEXT("Interactor")), Open->FindPinChecked(TEXT("Interactor")))
            && Link(Self->FindPinChecked(TEXT("self")), Open->FindPinChecked(TEXT("Target"))), SetupError);
    }
    if (!HasMarker(CG, TEXT("Minigame.Closed")))
    {
        auto* Property = FindFProperty<FObjectProperty>(Computer->GeneratedClass, TEXT("Minigame"));
        auto* Delegate = FindFProperty<FMulticastDelegateProperty>(UMinigameComponent::StaticClass(), TEXT("OnMinigameClosed"));
        REQUIRE_SETUP(Property && Delegate, TEXT("Minigame component closed event missing"));
        FGraphNodeCreator<UK2Node_ComponentBoundEvent> Creator(*CG);
        auto* Event = Creator.CreateNode(); Event->InitializeComponentBoundEventParams(Property, Delegate); Creator.Finalize();
        Event->NodePosX = 200; Event->NodePosY = 1700; Event->NodeComment = TEXT("Minigame.Closed");
        auto* Completed = Add<UK2Node_IfThenElse>(CG, 600, 1700);
        auto* Hide = Call(CG, Base->GeneratedClass, TEXT("SetAwarenessVisible"), 900, 1700);
        Hide->FindPinChecked(TEXT("IsVisible"))->DefaultValue = TEXT("false");
        REQUIRE_SETUP(Link(Event->FindPinChecked(TEXT("then")), Completed->GetExecPin()) && Link(Event->FindPinChecked(TEXT("bCompleted")), Completed->GetConditionPin()) && Link(Completed->GetThenPin(), Hide->GetExecPin()), SetupError);
        UEdGraphPin* Exec = Hide->GetThenPin();
        int32 X = 1300;
        for (FName Name : {FName(TEXT("Widget_InteractionPrompt")), FName(TEXT("Ring_Visual"))})
        {
            FGraphNodeCreator<UK2Node_VariableGet> GetCreator(*CG);
            auto* Get = GetCreator.CreateNode(); Get->VariableReference.SetSelfMember(Name); GetCreator.Finalize(); Get->NodePosX = X; Get->NodePosY = 1950;
            auto* Hidden = Call(CG, USceneComponent::StaticClass(), TEXT("SetHiddenInGame"), X, 1700);
            Hidden->FindPinChecked(TEXT("NewHidden"))->DefaultValue = TEXT("true");
            REQUIRE_SETUP(Link(Exec, Hidden->GetExecPin()) && Link(Get->FindPinChecked(Name), Hidden->FindPinChecked(TEXT("self"))), SetupError);
            Exec = Hidden->GetThenPin(); X += 400;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Computer);
    REQUIRE_SETUP(SuccessJSON(CompileAndSave(Computer, false)), TEXT("Computer integration compile failed"));
    REQUIRE_SETUP(Save(Widget) && Save(Computer), TEXT("Could not save wires integration"));
    return TEXT("{\"success\":true,\"blueprints\":2,\"levels_modified\":0}");
}
#undef REQUIRE_SETUP
