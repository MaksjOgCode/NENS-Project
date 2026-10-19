#include "InspectionEditorLibrary.h"
#include "InspectionBlueprintLibrary.h"
#include "MinigameComponent.h"
#include "MinigameWidget.h"
#include "ValvesMinigameWidget.h"
#include "BlueprintEditorLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/InheritableComponentHandler.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UnrealType.h"

// Shared checked graph/save primitives from InspectionSetup.cpp.
namespace InspectionSetup
{
extern FString SetupError;
bool Link(UEdGraphPin*,UEdGraphPin*);
UEdGraph* Graph(UBlueprint*,FName);
UEdGraphNode* Node(UEdGraph*,FName);
bool HasMarker(UEdGraph*,const FString&);
UK2Node_CallFunction* Call(UEdGraph*,UClass*,FName,int32,int32,const FString&);
bool SuccessJSON(const FString&);
FString Fail(const FString&);
bool Save(UObject*);
}
#define CHECK_VALVES(C,M) if (!(C)) return InspectionSetup::Fail(M)

FString UInspectionEditorLibrary::ConfigureValvesMinigame()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* Valve=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/MiniGameValves2/WBP_valves1.WBP_valves1"));
    auto* Board=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/MiniGameValves2/WBP_ValvesBoard.WBP_ValvesBoard"));
    auto* Box=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_PickMeBox.BP_Props_PickMeBox"));
    auto* Base=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Interface=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Interfaces/BPI_Interactable.BPI_Interactable"));
    auto* Sprite=LoadObject<UPaperSprite>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_PickMeBox.SPR_Props_PickMeBox"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    CHECK_VALVES(Valve && Board && Box && Base && Interface && Sprite && Font,TEXT("Required valve, box or shared interaction assets missing"));
    Valve->Modify();
    UBlueprintEditorLibrary::ReparentBlueprint(Valve,UValveControlWidget::StaticClass());
    auto* Image=Cast<UImage>(Valve->WidgetTree->FindWidget(TEXT("Image_76")));
    auto* Button=Cast<UButton>(Valve->WidgetTree->FindWidget(TEXT("Button_0")));
    CHECK_VALVES(Image && Button,TEXT("Original valve image/button changed"));
    // Crop transparent padding in the brush, preserving the source artwork.
    auto Brush=Image->GetBrush();
    Brush.SetUVRegion(FBox2f(FVector2f(.296f,.296f),FVector2f(.704f,.704f)));
    Image->SetBrush(Brush); Image->SetRenderScale(FVector2D(1,1));
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    FButtonStyle WheelStyle;
    FSlateBrush Empty; Empty.DrawAs=ESlateBrushDrawType::NoDrawType;
    WheelStyle.SetNormal(Empty); WheelStyle.SetHovered(Empty); WheelStyle.SetPressed(Empty);
    WheelStyle.SetNormalPadding(FMargin(0)); WheelStyle.SetPressedPadding(FMargin(0));
    Button->SetStyle(WheelStyle);
    auto* VG=Graph(Valve,TEXT("EventGraph"));
    CHECK_VALVES(VG,TEXT("Valve graph missing"));
    if (!HasMarker(VG,TEXT("Valves.InvalidateOnTurn")))
    {
        auto* SetAngle=Node(VG,TEXT("K2Node_VariableSet_0"));
        CHECK_VALVES(SetAngle,TEXT("Original valve angle setter changed"));
        auto* Exec=SetAngle->FindPinChecked(TEXT("execute"));
        auto Previous=Exec->LinkedTo; Exec->BreakAllPinLinks();
        FGraphNodeCreator<UK2Node_VariableSet> Creator(*VG);
        auto* Clear=Creator.CreateNode(); Clear->VariableReference.SetSelfMember(TEXT("IsUnlocked")); Creator.Finalize();
        Clear->NodeComment=TEXT("Valves.InvalidateOnTurn"); Clear->NodePosX=SetAngle->NodePosX-280; Clear->NodePosY=SetAngle->NodePosY;
        Clear->FindPinChecked(TEXT("IsUnlocked"))->DefaultValue=TEXT("false");
        CHECK_VALVES(Link(Clear->FindPinChecked(TEXT("then")),Exec),SetupError);
        for (auto* P : Previous) CHECK_VALVES(Link(P,Clear->FindPinChecked(TEXT("execute"))),SetupError);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Valve);
    CHECK_VALVES(SuccessJSON(CompileAndSave(Valve,false)),TEXT("Valve compile failed"));

    Board->Modify();
    UWidgetTree* Tree=Board->WidgetTree;
    auto* Root=Cast<UCanvasPanel>(Tree->RootWidget);
    auto* OriginalBoard=Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Board")));
    CHECK_VALVES(Root && OriginalBoard,TEXT("Original board layout changed"));
    for (FName Name : {TEXT("Valve_Left"),TEXT("Valve_Top"),TEXT("Valve_Right")})
    {
        auto* Wheel=Tree->FindWidget(Name);
        auto* Slot=Wheel ? Cast<UCanvasPanelSlot>(Wheel->Slot) : nullptr;
        CHECK_VALVES(Slot,TEXT("Original valve slot missing"));
        // Keep the original centers and apparent size, remove tiny 1px allocation hacks.
        Slot->SetSize(FVector2D(172,172)); Slot->SetAlignment(FVector2D(.5,.5));
        Wheel->SetRenderScale(FVector2D(.8,.8));
    }
    if (!Tree->FindWidget(TEXT("ScaleBox_Minigame")))
    {
        OriginalBoard->RemoveFromParent();
        auto* Fit=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ScaleBox_Minigame"));
        Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::Both);
        auto* FitSlot=Root->AddChildToCanvas(Fit);
        FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(32));
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("SizeBox_Minigame"));
        Size->SetWidthOverride(1800); Size->SetHeightOverride(1100);
        auto* Center=CastChecked<UScaleBoxSlot>(Fit->AddChild(Size));
        Center->SetHorizontalAlignment(HAlign_Center); Center->SetVerticalAlignment(VAlign_Center);
        auto* Scene=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Minigame"));
        Size->AddChild(Scene);
        auto Place=[Scene](UWidget* W,FVector2D Pos,FVector2D Dim)
        {
            auto* S=Scene->AddChildToCanvas(W); S->SetPosition(Pos); S->SetSize(Dim);
        };
        Place(OriginalBoard,FVector2D(100,112),FVector2D(1600,812));
        auto Text=[&](FName Name,const TCHAR* Copy,int32 Points,FVector2D Pos,FVector2D Dim)
        {
            auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
            T->SetText(FText::FromString(Copy)); T->SetFont(FSlateFontInfo(Font,Points,TEXT("Regular")));
            T->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1)));
            T->SetVisibility(ESlateVisibility::HitTestInvisible); Place(T,Pos,Dim); return T;
        };
        auto* Title=Text(TEXT("TxtTitle"),TEXT("Restore the flow"),46,FVector2D(100,24),FVector2D(1600,64));
        Title->SetJustification(ETextJustify::Center);
        auto* Status=Text(TEXT("TxtStatus"),TEXT("Aligned: 0 / 3"),28,FVector2D(100,944),FVector2D(1600,42));
        Status->SetJustification(ETextJustify::Center);
        auto* Controls=Text(TEXT("Text_ValvesControls"),
            TEXT("Left click — Turn a valve clockwise.\nMatch each white pointer to its diamond mark.\nEsc — Exit"),
            24,FVector2D(100,1000),FVector2D(1310,92));
        Controls->SetLineHeightPercentage(1.12f);
        Controls->SetColorAndOpacity(FSlateColor(FLinearColor(.7f,.69f,.65f,1)));
        auto* Close=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BtnClose"));
        Place(Close,FVector2D(1520,1014),FVector2D(180,54));
        auto* Exit=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("Text_Exit"));
        Exit->SetText(FText::FromString(TEXT("Exit"))); Exit->SetFont(FSlateFontInfo(Font,28,TEXT("Regular")));
        Exit->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); Close->AddChild(Exit);
        FButtonStyle Style;
        Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.035f,.034f,1),4.f,FLinearColor(.3f,.29f,.26f,1),1.f));
        Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.09f,.09f,.085f,1),4.f,FLinearColor(.6f,.58f,.52f,1),1.f));
        Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1),4.f));
        Style.SetNormalPadding(FMargin(12,4)); Style.SetPressedPadding(FMargin(12,5,12,3)); Close->SetStyle(Style);
        for (FName Name : {TEXT("ScaleBox_Minigame"),TEXT("SizeBox_Minigame"),TEXT("Canvas_Minigame"),TEXT("TxtTitle"),TEXT("TxtStatus"),TEXT("Text_ValvesControls"),TEXT("BtnClose"),TEXT("Text_Exit")})
            Board->WidgetVariableNameToGuidMap.Add(Name,FGuid::NewGuid());
    }
    UBlueprintEditorLibrary::ReparentBlueprint(Board,UValvesMinigameWidget::StaticClass());
    auto* BG=Graph(Board,TEXT("EventGraph"));
    CHECK_VALVES(BG,TEXT("Board event graph missing"));
    if (!HasMarker(BG,TEXT("Valves.ReportSuccess")))
    {
        auto* Delegate=Node(BG,TEXT("K2Node_CallDelegate_0"));
        CHECK_VALVES(Delegate,TEXT("Original board solved delegate changed"));
        auto* Exec=Delegate->FindPinChecked(TEXT("execute"));
        auto Previous=Exec->LinkedTo; Exec->BreakAllPinLinks();
        auto* Report=Call(BG,UMinigameWidget::StaticClass(),TEXT("ReportSuccess"),Delegate->NodePosX-300,Delegate->NodePosY,TEXT("Valves.ReportSuccess"));
        CHECK_VALVES(Report && Link(Report->GetThenPin(),Exec),SetupError);
        for (auto* P : Previous) CHECK_VALVES(Link(P,Report->GetExecPin()),SetupError);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Board);
    CHECK_VALVES(SuccessJSON(CompileAndSave(Board,false)),TEXT("Valve board compile failed"));

    Box->Modify();
    if (Box->ParentClass != Base->GeneratedClass)
    {
        CHECK_VALVES(Box->ParentClass==AActor::StaticClass(),TEXT("Box has custom parent; refusing to replace"));
        // Preserve this asset's identity and all placed references. Its only previous behavior is the sprite.
        auto OldNodes=Box->SimpleConstructionScript->GetAllNodes();
        for (int32 I=OldNodes.Num()-1;I>=0;--I) Box->SimpleConstructionScript->RemoveNode(OldNodes[I]);
        UBlueprintEditorLibrary::ReparentBlueprint(Box,Base->GeneratedClass);
    }
    auto* Handler=Box->GetInheritableComponentHandler(true);
    auto* VisualNode=Base->SimpleConstructionScript->FindSCSNode(TEXT("Sprite_Visual"));
    CHECK_VALVES(Handler && VisualNode,TEXT("Inherited sprite template missing"));
    FComponentKey Key(VisualNode);
    auto* Visual=Cast<UPaperSpriteComponent>(Handler->GetOverridenComponentTemplate(Key));
    if (!Visual) Visual=Cast<UPaperSpriteComponent>(Handler->CreateOverridenComponentTemplate(Key));
    CHECK_VALVES(Visual,TEXT("Could not create local sprite override"));
    Visual->SetSprite(Sprite); Visual->SetRelativeRotation(FRotator(0,90,-90));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    USimpleConstructionScript* SCS=Box->SimpleConstructionScript;
    auto* ComponentNode=SCS->FindSCSNode(TEXT("Minigame"));
    if (!ComponentNode) { ComponentNode=SCS->CreateNode(UMinigameComponent::StaticClass(),TEXT("Minigame")); SCS->AddNode(ComponentNode); }
    CastChecked<UMinigameComponent>(ComponentNode->ComponentTemplate)->WidgetClass=Board->GeneratedClass;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Box);
    CHECK_VALVES(SuccessJSON(CompileAndSave(Box,false)),TEXT("Box component compile failed"));
    auto* G=Graph(Box,TEXT("EventGraph"));
    CHECK_VALVES(G,TEXT("Box event graph missing"));
    if (!HasMarker(G,TEXT("Valves.Open")))
    {
        FGraphNodeCreator<UK2Node_Event> Creator(*G);
        auto* Interact=Creator.CreateNode();
        Interact->EventReference.SetExternalMember(TEXT("Interact"),Interface->GeneratedClass);
        Interact->bOverrideFunction=true; Creator.Finalize();
        Interact->NodePosX=0; Interact->NodePosY=0; Interact->NodeComment=TEXT("Valves.Open");
        auto* Open=Call(G,UInspectionBlueprintLibrary::StaticClass(),TEXT("OpenMinigame"),350,0,TEXT("Valves.OpenCall"));
        FGraphNodeCreator<UK2Node_Self> SelfCreator(*G); auto* Self=SelfCreator.CreateNode(); SelfCreator.Finalize();
        CHECK_VALVES(Link(Interact->FindPinChecked(TEXT("then")),Open->GetExecPin())
            && Link(Interact->FindPinChecked(TEXT("Interactor")),Open->FindPinChecked(TEXT("Interactor")))
            && Link(Self->FindPinChecked(TEXT("self")),Open->FindPinChecked(TEXT("Target"))),SetupError);
    }
    if (!HasMarker(G,TEXT("Valves.Closed")))
    {
        auto* Property=FindFProperty<FObjectProperty>(Box->GeneratedClass,TEXT("Minigame"));
        auto* Delegate=FindFProperty<FMulticastDelegateProperty>(UMinigameComponent::StaticClass(),TEXT("OnMinigameClosed"));
        CHECK_VALVES(Property && Delegate,TEXT("Box minigame closed event missing"));
        FGraphNodeCreator<UK2Node_ComponentBoundEvent> Creator(*G);
        auto* Event=Creator.CreateNode(); Event->InitializeComponentBoundEventParams(Property,Delegate); Creator.Finalize();
        Event->NodeComment=TEXT("Valves.Closed"); Event->NodePosY=500;
        FGraphNodeCreator<UK2Node_IfThenElse> BC(*G); auto* Branch=BC.CreateNode(); BC.Finalize(); Branch->NodePosX=350; Branch->NodePosY=500;
        auto* Hide=Call(G,Base->GeneratedClass,TEXT("SetAwarenessVisible"),650,500,TEXT(""));
        Hide->FindPinChecked(TEXT("IsVisible"))->DefaultValue=TEXT("false");
        CHECK_VALVES(Link(Event->FindPinChecked(TEXT("then")),Branch->GetExecPin())
            && Link(Event->FindPinChecked(TEXT("bCompleted")),Branch->GetConditionPin())
            && Link(Branch->GetThenPin(),Hide->GetExecPin()),SetupError);
        UEdGraphPin* Exec=Hide->GetThenPin();
        int32 X=1000;
        for (FName Name : {TEXT("Widget_InteractionPrompt"),TEXT("Ring_Visual")})
        {
            FGraphNodeCreator<UK2Node_VariableGet> GC(*G);
            auto* Get=GC.CreateNode(); Get->VariableReference.SetSelfMember(Name); GC.Finalize();
            Get->NodePosX=X; Get->NodePosY=800;
            auto* Hidden=Call(G,USceneComponent::StaticClass(),TEXT("SetHiddenInGame"),X,500,TEXT(""));
            Hidden->FindPinChecked(TEXT("NewHidden"))->DefaultValue=TEXT("true");
            CHECK_VALVES(Link(Exec,Hidden->GetExecPin()) && Link(Get->FindPinChecked(Name),Hidden->FindPinChecked(TEXT("self"))),SetupError);
            Exec=Hidden->GetThenPin(); X+=400;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Box);
    CHECK_VALVES(SuccessJSON(CompileAndSave(Box,false)),TEXT("Box integration compile failed"));
    CHECK_VALVES(Save(Valve) && Save(Board) && Save(Box),TEXT("Could not save valve integration"));
    return TEXT("{\"success\":true,\"blueprints\":3,\"levels_modified\":0}");
}
#undef CHECK_VALVES
