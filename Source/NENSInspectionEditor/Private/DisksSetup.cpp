#include "InspectionEditorLibrary.h"
#include "InspectionBlueprintLibrary.h"
#include "DisksMinigameWidget.h"
#include "MinigameComponent.h"
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
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Font.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "UObject/UnrealType.h"

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
#define CHECK_DISKS(C,M) if (!(C)) return InspectionSetup::Fail(M)

FString UInspectionEditorLibrary::ConfigureDisksMinigame()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/Minigames/Disks/WBP_DiskBoard.WBP_DiskBoard"));
    auto* Cabinet=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_BedsideCabinet.BP_BedsideCabinet"));
    auto* Base=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Interface=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Interfaces/BPI_Interactable.BPI_Interactable"));
    auto* Sprite=LoadObject<UPaperSprite>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_MetalCabinet.SPR_Props_MetalCabinet"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    auto* Background=LoadObject<UTexture2D>(nullptr,TEXT("/Game/NENS/UI/Inspection/Textures/T_Inspection_Container.T_Inspection_Container"));
    CHECK_DISKS(BP && Cabinet && Base && Interface && Sprite && Font && Background,TEXT("Required disks/cabinet/inspection assets missing"));
    BP->Modify();
    UWidgetTree* Tree=BP->WidgetTree;
    auto* Root=Cast<UCanvasPanel>(Tree->RootWidget);
    auto* Board=Cast<UCanvasPanel>(Tree->FindWidget(TEXT("Board")));
    auto* DiskButton=Cast<UButton>(Tree->FindWidget(TEXT("Btn_Disks")));
    CHECK_DISKS(Root && Board && DiskButton,TEXT("Original disk layout changed"));
    auto* Focusable=FindFProperty<FBoolProperty>(UButton::StaticClass(),TEXT("IsFocusable"));
    CHECK_DISKS(Focusable,TEXT("Button focus property missing"));
    Focusable->SetPropertyValue_InContainer(DiskButton,false);
    // The click surface is transparent; selected rings remain visible while held.
    FSlateBrush Empty; Empty.DrawAs=ESlateBrushDrawType::NoDrawType;
    FButtonStyle Clear; Clear.SetNormal(Empty); Clear.SetHovered(Empty); Clear.SetPressed(Empty);
    Clear.SetNormalPadding(FMargin(0)); Clear.SetPressedPadding(FMargin(0)); DiskButton->SetStyle(Clear);
    for (FName Name : {TEXT("Img_Bg"),TEXT("Ring_Outer"),TEXT("Ring_Middle"),TEXT("Ring_Inner")})
        if (auto* Art=Tree->FindWidget(Name)) Art->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (!Tree->FindWidget(TEXT("ScaleBox_Minigame")))
    {
        Board->RemoveFromParent();
        auto* Fit=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ScaleBox_Minigame"));
        Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::Both);
        auto* FrameSlot=Root->AddChildToCanvas(Fit);
        FrameSlot->SetAnchors(FAnchors(0,0,1,1)); FrameSlot->SetOffsets(FMargin(32));
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("SizeBox_Minigame"));
        Size->SetWidthOverride(1800); Size->SetHeightOverride(1240);
        auto* FitContent=CastChecked<UScaleBoxSlot>(Fit->AddChild(Size));
        FitContent->SetHorizontalAlignment(HAlign_Center); FitContent->SetVerticalAlignment(VAlign_Center);
        auto* Scene=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Minigame"));
        Size->AddChild(Scene);
        auto Place=[](UCanvasPanel* Parent,UWidget* W,FVector2D Pos,FVector2D Dim)
        {
            auto* S=Parent->AddChildToCanvas(W); S->SetPosition(Pos); S->SetSize(Dim);
        };
        auto* Pages=Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("ScenePages"));
        Place(Scene,Pages,FVector2D(100,112),FVector2D(1600,1000));
        auto* Inspect=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Container"));
        auto* Puzzle=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Disks"));
        Pages->AddChild(Inspect); Pages->AddChild(Puzzle);
        for (auto* Page : {Inspect,Puzzle})
        {
            auto* S=CastChecked<UWidgetSwitcherSlot>(Page->Slot); S->SetHorizontalAlignment(HAlign_Fill); S->SetVerticalAlignment(VAlign_Fill);
        }
        Pages->SetActiveWidgetIndex(1); // Designer and the standalone test level show the puzzle.
        Place(Puzzle,Board,FVector2D(300,0),FVector2D(1000,1000));
        // NullRHI imports may not have a render resource yet; source dimensions are authoritative.
        CHECK_DISKS(Background->Source.GetSizeX()>0 && Background->Source.GetSizeY()>0,TEXT("Inspection image source dimensions missing"));
        const double H=1600.*Background->Source.GetSizeY()/Background->Source.GetSizeX(), Y=(1000-H)*.5;
        auto* Art=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Image_Container"));
        Art->SetBrushFromTexture(Background); Art->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Inspect,Art,FVector2D(0,Y),FVector2D(1600,H));
        auto* Hotspot=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("Btn_Cabinet"));
        Place(Inspect,Hotspot,FVector2D(1600*.665,Y+H*.245),FVector2D(1600*.22,H*.40));
        Focusable->SetPropertyValue_InContainer(Hotspot,false); Hotspot->SetCursor(EMouseCursor::Hand);
        FButtonStyle Hover=Clear;
        Hover.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.9f,.9f,.86f,.045f),12.f,FLinearColor(.9f,.89f,.84f,.4f),2.f));
        Hover.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.9f,.9f,.86f,.07f),12.f,FLinearColor(.9f,.89f,.84f,.6f),2.f));
        Hotspot->SetStyle(Hover);
        auto Text=[&](FName Name,const TCHAR* Copy,int32 Points,FVector2D Pos,FVector2D Dim)
        {
            auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
            T->SetText(FText::FromString(Copy)); T->SetFont(FSlateFontInfo(Font,Points,TEXT("Regular")));
            T->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); T->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(Scene,T,Pos,Dim); return T;
        };
        auto* Title=Text(TEXT("TxtTitle"),TEXT("Restore the image"),46,FVector2D(100,14),FVector2D(1600,64)); Title->SetJustification(ETextJustify::Center);
        auto* Status=Text(TEXT("TxtStatus"),TEXT(""),26,FVector2D(100,78),FVector2D(1600,34)); Status->SetJustification(ETextJustify::Center);
        auto* Controls=Text(TEXT("Text_DisksControls"),TEXT("Hold left click — Turn\nEsc — Exit"),26,FVector2D(100,1146),FVector2D(1300,80));
        Controls->SetLineHeightPercentage(1.12f); Controls->SetColorAndOpacity(FSlateColor(FLinearColor(.7f,.69f,.65f,1)));
        auto* Exit=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("BtnClose"));
        Place(Scene,Exit,FVector2D(1520,1158),FVector2D(180,54));
        auto* ExitText=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("Text_Exit"));
        ExitText->SetText(FText::FromString(TEXT("Exit"))); ExitText->SetFont(FSlateFontInfo(Font,28,TEXT("Regular")));
        ExitText->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); Exit->AddChild(ExitText);
        FButtonStyle Style;
        Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.035f,.034f,1),4.f,FLinearColor(.3f,.29f,.26f,1),1.f));
        Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.09f,.09f,.085f,1),4.f,FLinearColor(.6f,.58f,.52f,1),1.f));
        Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1),4.f));
        Style.SetNormalPadding(FMargin(12,4)); Style.SetPressedPadding(FMargin(12,5,12,3)); Exit->SetStyle(Style);
        for (FName Name : {TEXT("ScaleBox_Minigame"),TEXT("SizeBox_Minigame"),TEXT("Canvas_Minigame"),TEXT("ScenePages"),TEXT("Canvas_Container"),TEXT("Canvas_Disks"),TEXT("Image_Container"),TEXT("Btn_Cabinet"),TEXT("TxtTitle"),TEXT("TxtStatus"),TEXT("Text_DisksControls"),TEXT("BtnClose"),TEXT("Text_Exit")})
            BP->WidgetVariableNameToGuidMap.Add(Name,FGuid::NewGuid());
    }
    CHECK_DISKS(Background->Source.GetSizeX()>0 && Background->Source.GetSizeY()>0,TEXT("Inspection image source dimensions missing"));
    const double ImageH=1600.*Background->Source.GetSizeY()/Background->Source.GetSizeX(), ImageY=(1000-ImageH)*.5;
    for (FName Name : {TEXT("Image_Container"),TEXT("Btn_Cabinet")})
    {
        auto* W=Tree->FindWidget(Name); auto* S=W ? Cast<UCanvasPanelSlot>(W->Slot) : nullptr;
        CHECK_DISKS(S,TEXT("Container image or hotspot slot missing"));
        const bool IsArt=Name==TEXT("Image_Container");
        S->SetPosition(IsArt ? FVector2D(0,ImageY) : FVector2D(1600*.665,ImageY+ImageH*.245));
        S->SetSize(IsArt ? FVector2D(1600,ImageH) : FVector2D(1600*.22,ImageH*.40));
    }
    UBlueprintEditorLibrary::ReparentBlueprint(BP,UDisksMinigameWidget::StaticClass());
    auto* G=Graph(BP,TEXT("EventGraph"));
    CHECK_DISKS(G,TEXT("Disk graph missing"));
    if (!HasMarker(G,TEXT("Disks.PointerCoordinates")))
    {
        auto* Mouse=Node(G,TEXT("K2Node_CallFunction_0"));
        CHECK_DISKS(Mouse && Mouse->FindPin(TEXT("ReturnValue")),TEXT("Disk pointer getter changed"));
        auto* Position=Call(G,UDisksMinigameWidget::StaticClass(),TEXT("GetDiskPressPosition"),100,500,TEXT("Disks.PointerCoordinates"));
        CHECK_DISKS(Position,TEXT("Native disk position function missing"));
        auto* Output=Mouse->FindPinChecked(TEXT("ReturnValue")); auto Consumers=Output->LinkedTo; Output->BreakAllPinLinks();
        for (auto* P : Consumers) CHECK_DISKS(Link(Position->GetReturnValuePin(),P),SetupError);
    }
    if (!HasMarker(G,TEXT("Disks.ReportSuccess")))
    {
        auto* Win=Node(G,TEXT("K2Node_IfThenElse_4"));
        auto* Disable=Node(G,TEXT("K2Node_CallFunction_3"));
        auto* Delay=Node(G,TEXT("K2Node_CallFunction_4"));
        CHECK_DISKS(Win && Disable && Delay,TEXT("Original disk victory sequence changed"));
        auto* Report=Call(G,UDisksMinigameWidget::StaticClass(),TEXT("CompleteDiskPuzzle"),Win->NodePosX+300,Win->NodePosY,TEXT("Disks.ReportSuccess"));
        Win->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
        Disable->FindPinChecked(TEXT("execute"))->BreakAllPinLinks();
        Disable->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
        Delay->FindPinChecked(TEXT("execute"))->BreakAllPinLinks();
        CHECK_DISKS(Link(Win->FindPinChecked(TEXT("then")),Report->GetExecPin())
            && Link(Report->GetThenPin(),Disable->FindPinChecked(TEXT("execute")))
            && Link(Disable->FindPinChecked(TEXT("then")),Delay->FindPinChecked(TEXT("execute"))),SetupError);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    CHECK_DISKS(SuccessJSON(CompileAndSave(BP,false)),TEXT("Adapted disk widget compile failed"));

    CHECK_DISKS(Cabinet->ParentClass==Base->GeneratedClass,TEXT("Cabinet must inherit the shared interaction base"));
    Cabinet->Modify();
    auto* Handler=Cabinet->GetInheritableComponentHandler(true);
    auto* VisualNode=Base->SimpleConstructionScript->FindSCSNode(TEXT("Sprite_Visual"));
    CHECK_DISKS(Handler && VisualNode,TEXT("Cabinet sprite template missing"));
    FComponentKey Key(VisualNode);
    auto* Visual=Cast<UPaperSpriteComponent>(Handler->GetOverridenComponentTemplate(Key));
    if (!Visual) Visual=Cast<UPaperSpriteComponent>(Handler->CreateOverridenComponentTemplate(Key));
    CHECK_DISKS(Visual,TEXT("Cabinet sprite override failed"));
    Visual->SetSprite(Sprite); Visual->SetRelativeRotation(FRotator(0,90,-90)); Visual->SetRelativeScale3D(FVector(.45));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    USimpleConstructionScript* SCS=Cabinet->SimpleConstructionScript;
    auto* ComponentNode=SCS->FindSCSNode(TEXT("Minigame"));
    if (!ComponentNode) { ComponentNode=SCS->CreateNode(UMinigameComponent::StaticClass(),TEXT("Minigame")); SCS->AddNode(ComponentNode); }
    CastChecked<UMinigameComponent>(ComponentNode->ComponentTemplate)->WidgetClass=BP->GeneratedClass;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Cabinet);
    CHECK_DISKS(SuccessJSON(CompileAndSave(Cabinet,false)),TEXT("Cabinet component compile failed"));
    auto* CG=Graph(Cabinet,TEXT("EventGraph"));
    if (!CG)
    {
        CG=FBlueprintEditorUtils::CreateNewGraph(Cabinet,TEXT("EventGraph"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddUbergraphPage(Cabinet,CG);
    }
    if (!HasMarker(CG,TEXT("Disks.Open")))
    {
        FGraphNodeCreator<UK2Node_Event> EC(*CG);
        auto* Event=EC.CreateNode(); Event->EventReference.SetExternalMember(TEXT("Interact"),Interface->GeneratedClass); Event->bOverrideFunction=true; EC.Finalize();
        Event->NodeComment=TEXT("Disks.Open");
        auto* Open=Call(CG,UInspectionBlueprintLibrary::StaticClass(),TEXT("OpenMinigame"),350,0,TEXT(""));
        FGraphNodeCreator<UK2Node_Self> SC(*CG); auto* Self=SC.CreateNode(); SC.Finalize();
        CHECK_DISKS(Link(Event->FindPinChecked(TEXT("then")),Open->GetExecPin())
            && Link(Event->FindPinChecked(TEXT("Interactor")),Open->FindPinChecked(TEXT("Interactor")))
            && Link(Self->FindPinChecked(TEXT("self")),Open->FindPinChecked(TEXT("Target"))),SetupError);
    }
    if (!HasMarker(CG,TEXT("Disks.Closed")))
    {
        auto* Property=FindFProperty<FObjectProperty>(Cabinet->GeneratedClass,TEXT("Minigame"));
        auto* Delegate=FindFProperty<FMulticastDelegateProperty>(UMinigameComponent::StaticClass(),TEXT("OnMinigameClosed"));
        CHECK_DISKS(Property && Delegate,TEXT("Cabinet closed delegate missing"));
        FGraphNodeCreator<UK2Node_ComponentBoundEvent> EC(*CG);
        auto* Event=EC.CreateNode(); Event->InitializeComponentBoundEventParams(Property,Delegate); EC.Finalize();
        Event->NodeComment=TEXT("Disks.Closed"); Event->NodePosY=500;
        FGraphNodeCreator<UK2Node_IfThenElse> BC(*CG); auto* Branch=BC.CreateNode(); BC.Finalize();
        Branch->NodePosX=350; Branch->NodePosY=500;
        auto* Hide=Call(CG,Base->GeneratedClass,TEXT("SetAwarenessVisible"),650,500,TEXT(""));
        Hide->FindPinChecked(TEXT("IsVisible"))->DefaultValue=TEXT("false");
        CHECK_DISKS(Link(Event->FindPinChecked(TEXT("then")),Branch->GetExecPin())
            && Link(Event->FindPinChecked(TEXT("bCompleted")),Branch->GetConditionPin())
            && Link(Branch->GetThenPin(),Hide->GetExecPin()),SetupError);
        UEdGraphPin* Exec=Hide->GetThenPin(); int32 X=1000;
        for (FName Name : {TEXT("Widget_InteractionPrompt"),TEXT("Ring_Visual")})
        {
            FGraphNodeCreator<UK2Node_VariableGet> GC(*CG);
            auto* Get=GC.CreateNode(); Get->VariableReference.SetSelfMember(Name); GC.Finalize(); Get->NodePosX=X; Get->NodePosY=800;
            auto* Hidden=Call(CG,USceneComponent::StaticClass(),TEXT("SetHiddenInGame"),X,500,TEXT(""));
            Hidden->FindPinChecked(TEXT("NewHidden"))->DefaultValue=TEXT("true");
            CHECK_DISKS(Link(Exec,Hidden->GetExecPin()) && Link(Get->FindPinChecked(Name),Hidden->FindPinChecked(TEXT("self"))),SetupError);
            Exec=Hidden->GetThenPin(); X+=400;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Cabinet);
    CHECK_DISKS(SuccessJSON(CompileAndSave(Cabinet,false)),TEXT("Cabinet integration compile failed"));
    CHECK_DISKS(Save(BP) && Save(Cabinet),TEXT("Could not save disks and cabinet"));
    return TEXT("{\"success\":true,\"blueprints\":2,\"levels_modified\":0}");
}
#undef CHECK_DISKS
