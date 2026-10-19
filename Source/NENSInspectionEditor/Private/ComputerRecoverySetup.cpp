#include "InspectionEditorLibrary.h"
#include "InspectionBlueprintLibrary.h"
#include "ComputerRecoveryData.h"
#include "ComputerRecoveryComponent.h"
#include "ComputerRecoveryWidget.h"
#include "MinigameComponent.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintEditorLibrary.h"
#include "WidgetBlueprint.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/InheritableComponentHandler.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_Self.h"
#include "K2Node_CallFunction.h"
#include "UObject/UnrealType.h"
namespace InspectionSetup
{
extern FString SetupError;
bool Link(UEdGraphPin*,UEdGraphPin*);
UEdGraph* Graph(UBlueprint*,FName);
bool HasMarker(UEdGraph*,const FString&);
UK2Node_CallFunction* Call(UEdGraph*,UClass*,FName,int32,int32,const FString&);
bool SuccessJSON(const FString&);
FString Fail(const FString&);
bool Save(UObject*);
}
#define CHECK_RECOVERY(C,M) if (!(C)) return InspectionSetup::Fail(M)

FString UInspectionEditorLibrary::ConfigureComputerRecovery()
{
    using namespace InspectionSetup;
    SetupError.Reset();
    auto* Widget=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/NENS/UI/Recovery/WBP_ComputerRecovery.WBP_ComputerRecovery"));
    auto* Data=LoadObject<UComputerRecoveryData>(nullptr,TEXT("/Game/NENS/UI/Recovery/DA_ComputerRecovery.DA_ComputerRecovery"));
    auto* Computer=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_DataComputer.BP_Props_DataComputer"));
    auto* Base=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Interface=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Interfaces/BPI_Interactable.BPI_Interactable"));
    auto* Sprite=LoadObject<UPaperSprite>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_DataComputer.SPR_Props_DataComputer"));
    auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Textures/T_Props_DataComputer.T_Props_DataComputer"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    TSet<FName> Cards;
    CHECK_RECOVERY(Widget && Data && Computer && Base && Interface && Sprite && Texture && Font && Data->GetRequiredCards(Cards),TEXT("Recovery assets or card definitions missing/invalid"));
    Widget->Modify(); UWidgetTree* Tree=Widget->WidgetTree;
    if (!Tree->FindWidget(TEXT("ScaleBox_Minigame")))
    {
        auto* Root=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Root")); Tree->RootWidget=Root;
        auto* Fit=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ScaleBox_Minigame"));
        Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::Both);
        auto* FrameSlot=Root->AddChildToCanvas(Fit); FrameSlot->SetAnchors(FAnchors(0,0,1,1)); FrameSlot->SetOffsets(FMargin(32));
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("SizeBox_Minigame")); Size->SetWidthOverride(1560); Size->SetHeightOverride(1120);
        auto* FitSlot=CastChecked<UScaleBoxSlot>(Fit->AddChild(Size)); FitSlot->SetHorizontalAlignment(HAlign_Center); FitSlot->SetVerticalAlignment(VAlign_Center);
        auto* Scene=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Minigame")); Size->AddChild(Scene);
        auto Place=[](UCanvasPanel* Parent,UWidget* W,FVector2D P,FVector2D S) { auto* Slot=Parent->AddChildToCanvas(W); Slot->SetPosition(P); Slot->SetSize(S); };
        auto Text=[&](UCanvasPanel* Parent,FName Name,const TCHAR* Copy,int32 Points,FVector2D P,FVector2D S,FLinearColor C=FLinearColor(.9f,.88f,.82f,1))
        {
            auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name); T->SetText(FText::FromString(Copy));
            T->SetFont(FSlateFontInfo(Font,Points,TEXT("Regular"))); T->SetColorAndOpacity(FSlateColor(C)); T->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(Parent,T,P,S); return T;
        };
        Text(Scene,TEXT("Txt_Eyebrow"),TEXT("THE MAIN COMPUTER"),18,FVector2D(80,20),FVector2D(1400,28),FLinearColor(.58f,.58f,.54f,1));
        Text(Scene,TEXT("TxtTitle"),TEXT("System recovery"),48,FVector2D(80,56),FVector2D(1400,70));
        Text(Scene,TEXT("Txt_Description"),TEXT("Restore the systems. Load the punch cards. Bring the computer back online."),24,FVector2D(80,136),FVector2D(1400,56));
        Text(Scene,TEXT("Txt_State"),TEXT("SYSTEMS OFFLINE"),30,FVector2D(80,220),FVector2D(580,50));
        auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Image_Computer")); Image->SetBrushFromTexture(Texture);
        auto Brush=Image->GetBrush(); Brush.SetUVRegion(FBox2f(FVector2f(.132f,.262f),FVector2f(.915f,.639f))); Image->SetBrush(Brush);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Scene,Image,FVector2D(80,325),FVector2D(580,580.*377/783));
        Text(Scene,TEXT("Txt_Goal"),TEXT("Find the cards.\nRestore the systems.\nRun the recovery program."),26,FVector2D(80,650),FVector2D(580,124));
        Text(Scene,TEXT("Txt_ProgressLabel"),TEXT("SYSTEM READINESS"),18,FVector2D(80,802),FVector2D(450,32),FLinearColor(.58f,.58f,.54f,1));
        auto* Percent=Text(Scene,TEXT("Txt_ProgressValue"),TEXT("0%"),20,FVector2D(530,800),FVector2D(130,34)); Percent->SetJustification(ETextJustify::Right);
        auto* Progress=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("RecoveryProgress"));
        FProgressBarStyle ProgressStyle;
        FSlateBrush Back; Back.TintColor=FSlateColor(FLinearColor(.03f,.03f,.03f,1)); Back.DrawAs=ESlateBrushDrawType::Image;
        FSlateBrush Fill=Back; Fill.TintColor=FSlateColor(FLinearColor(.8f,.78f,.7f,1)); ProgressStyle.SetBackgroundImage(Back); ProgressStyle.SetFillImage(Fill);
        Progress->SetWidgetStyle(ProgressStyle); Progress->SetFillColorAndOpacity(FLinearColor::White); Progress->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Scene,Progress,FVector2D(80,848),FVector2D(580,12));
        auto* Hint=Text(Scene,TEXT("Txt_NextStep"),TEXT("Connect the wires to restore power."),24,FVector2D(80,888),FVector2D(580,100)); Hint->SetAutoWrapText(true);
        const TCHAR* Keys[]={TEXT("Power"),TEXT("Fuel"),TEXT("Display"),TEXT("Program")};
        const TCHAR* Labels[]={TEXT("POWER SUPPLY"),TEXT("FUEL SUPPLY"),TEXT("DISPLAY"),TEXT("RECOVERY PROGRAM")};
        const TCHAR* Hints[]={TEXT("Connect the wires to supply electricity."),TEXT("Turn the valves to restore the fuel flow."),TEXT("Align the disks to calibrate the image."),TEXT("Find the punch cards and load them into the reader.")};
        for (int32 I=0;I<4;++I)
        {
            auto* Border=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),FName(*FString::Printf(TEXT("Panel_%s"),Keys[I])));
            Border->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.023f,.024f,.025f,.94f),6.f,FLinearColor(.17f,.17f,.16f,1),1.f)); Border->SetPadding(FMargin(0));
            Place(Scene,Border,FVector2D(720,210+I*164),FVector2D(760,148));
            auto* Row=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),FName(*FString::Printf(TEXT("Row_%s"),Keys[I]))); Border->AddChild(Row);
            Text(Row,FName(*FString::Printf(TEXT("Txt_%sLabel"),Keys[I])),Labels[I],20,FVector2D(24,18),FVector2D(470,34),FLinearColor(.62f,.61f,.57f,1));
            auto* Status=Text(Row,FName(*FString::Printf(TEXT("Txt_%sStatus"),Keys[I])),I<3 ? TEXT("OFFLINE") : TEXT("0 / 2 LOADED"),24,FVector2D(490,18),FVector2D(246,36));
            Status->SetJustification(ETextJustify::Right);
            auto* Copy=Text(Row,FName(*FString::Printf(TEXT("Txt_%sHint"),Keys[I])),Hints[I],24,FVector2D(24,64),FVector2D(712,76)); Copy->SetAutoWrapText(true);
        }
        FButtonStyle ButtonStyle;
        ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.035f,.034f,1),4.f,FLinearColor(.38f,.37f,.32f,1),1.f));
        ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.12f,.12f,.11f,1),4.f,FLinearColor(.7f,.68f,.59f,1),1.f));
        ButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1),4.f));
        ButtonStyle.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.025f,.025f,.025f,.7f),4.f,FLinearColor(.12f,.12f,.12f,1),1.f));
        ButtonStyle.SetNormalPadding(FMargin(12,4)); ButtonStyle.SetPressedPadding(FMargin(12,5,12,3));
        auto Button=[&](FName Name,const TCHAR* Label,FVector2D P,FVector2D S)
        {
            auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name); B->SetStyle(ButtonStyle); Place(Scene,B,P,S);
            auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*(Name.ToString()+TEXT("_Text"))));
            T->SetText(FText::FromString(Label)); T->SetFont(FSlateFontInfo(Font,26,TEXT("Regular"))); T->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); B->AddChild(T);
        };
        Button(TEXT("Btn_LoadCards"),TEXT("Load punch cards"),FVector2D(720,886),FVector2D(360,64));
        Button(TEXT("Btn_Restore"),TEXT("Restore computer"),FVector2D(1120,886),FVector2D(360,64));
        Button(TEXT("BtnClose"),TEXT("Exit"),FVector2D(1300,1020),FVector2D(180,54));
        Text(Scene,TEXT("Text_RecoveryControls"),TEXT("Left click — Use terminal\nEsc — Exit"),22,FVector2D(80,1020),FVector2D(1000,68),FLinearColor(.65f,.64f,.6f,1));
        Tree->ForEachWidget([Widget](UWidget* W) { Widget->WidgetVariableNameToGuidMap.FindOrAdd(W->GetFName())=FGuid::NewGuid(); });
    }
    if (Widget->ParentClass != UComputerRecoveryWidget::StaticClass()) UBlueprintEditorLibrary::ReparentBlueprint(Widget,UComputerRecoveryWidget::StaticClass());
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
    CHECK_RECOVERY(SuccessJSON(CompileAndSave(Widget,false)),TEXT("Recovery widget compile failed"));

    Computer->Modify();
    if (Computer->ParentClass != Base->GeneratedClass)
    {
        CHECK_RECOVERY(Computer->ParentClass==AActor::StaticClass(),TEXT("Computer already has a custom parent"));
        auto OldNodes=Computer->SimpleConstructionScript->GetAllNodes();
        for (int32 I=OldNodes.Num()-1;I>=0;--I) Computer->SimpleConstructionScript->RemoveNode(OldNodes[I]);
        UBlueprintEditorLibrary::ReparentBlueprint(Computer,Base->GeneratedClass);
    }
    auto* Handler=Computer->GetInheritableComponentHandler(true);
    auto Override=[&](FName Name)
    {
        auto* Node=Base->SimpleConstructionScript->FindSCSNode(Name);
        if (!Node) return static_cast<UActorComponent*>(nullptr);
        FComponentKey Key(Node);
        auto* C=Handler->GetOverridenComponentTemplate(Key);
        return C ? C : Handler->CreateOverridenComponentTemplate(Key);
    };
    auto* Visual=Cast<UPaperSpriteComponent>(Override(TEXT("Sprite_Visual")));
    CHECK_RECOVERY(Visual,TEXT("Computer sprite override missing"));
    Visual->SetSprite(Sprite); Visual->SetRelativeRotation(FRotator(0,90,-90)); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Interaction=Cast<USphereComponent>(Override(TEXT("Sphere_Interaction")));
    auto* Awareness=Cast<USphereComponent>(Override(TEXT("Sphere_Awareness")));
    auto* Ring=Cast<USceneComponent>(Override(TEXT("Ring_Visual")));
    CHECK_RECOVERY(Interaction && Awareness && Ring,TEXT("Computer interaction template missing"));
    Interaction->SetSphereRadius(90); Awareness->SetSphereRadius(190);
    Ring->SetRelativeScale3D(FVector(1.8,1.8,1)); Ring->SetRelativeLocation(FVector(0,0,1));
    USimpleConstructionScript* SCS=Computer->SimpleConstructionScript;
    auto* RecoveryNode=SCS->FindSCSNode(TEXT("Recovery"));
    if (!RecoveryNode) { RecoveryNode=SCS->CreateNode(UComputerRecoveryComponent::StaticClass(),TEXT("Recovery")); SCS->AddNode(RecoveryNode); }
    auto* Component=CastChecked<UComputerRecoveryComponent>(RecoveryNode->ComponentTemplate);
    Component->Definition=Data; Component->WidgetClass=Widget->GeneratedClass;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Computer);
    CHECK_RECOVERY(SuccessJSON(CompileAndSave(Computer,false)),TEXT("Recovery component compile failed"));
    auto* G=Graph(Computer,TEXT("EventGraph"));
    CHECK_RECOVERY(G,TEXT("Computer event graph missing"));
    if (!HasMarker(G,TEXT("Recovery.Open")))
    {
        FGraphNodeCreator<UK2Node_Event> EC(*G); auto* Event=EC.CreateNode();
        Event->EventReference.SetExternalMember(TEXT("Interact"),Interface->GeneratedClass); Event->bOverrideFunction=true; EC.Finalize();
        Event->NodeComment=TEXT("Recovery.Open");
        auto* Open=Call(G,UInspectionBlueprintLibrary::StaticClass(),TEXT("OpenMainComputer"),350,0,TEXT(""));
        FGraphNodeCreator<UK2Node_Self> SC(*G); auto* Self=SC.CreateNode(); SC.Finalize();
        CHECK_RECOVERY(Link(Event->FindPinChecked(TEXT("then")),Open->GetExecPin()) && Link(Event->FindPinChecked(TEXT("Interactor")),Open->FindPinChecked(TEXT("Interactor"))) && Link(Self->FindPinChecked(TEXT("self")),Open->FindPinChecked(TEXT("Target"))),SetupError);
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Computer);
    CHECK_RECOVERY(SuccessJSON(CompileAndSave(Computer,false)),TEXT("Computer interaction compile failed"));
    const TPair<const TCHAR*,EComputerSystem> Stations[]={
        {TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_ComputerCabinet.BP_ComputerCabinet"),EComputerSystem::Power},
        {TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_PickMeBox.BP_Props_PickMeBox"),EComputerSystem::Fuel},
        {TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_BedsideCabinet.BP_BedsideCabinet"),EComputerSystem::Display}};
    TArray<UBlueprint*> SaveList={Widget,Computer};
    for (const auto& Station : Stations)
    {
        auto* BP=LoadObject<UBlueprint>(nullptr,Station.Key);
        auto* Node=BP ? BP->SimpleConstructionScript->FindSCSNode(TEXT("Minigame")) : nullptr;
        auto* Game=Node ? Cast<UMinigameComponent>(Node->ComponentTemplate) : nullptr;
        CHECK_RECOVERY(Game,TEXT("Existing puzzle component missing"));
        BP->Modify(); Game->RepairedSystem=Station.Value;
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        CHECK_RECOVERY(SuccessJSON(CompileAndSave(BP,false)),TEXT("Puzzle recovery assignment compile failed"));
        SaveList.Add(BP);
    }
    for (auto* BP : SaveList) CHECK_RECOVERY(Save(BP),TEXT("Could not save recovery Blueprint"));
    return FString::Printf(TEXT("{\"success\":true,\"blueprints\":5,\"required_cards\":%d,\"levels_modified\":0}"),Cards.Num());
}
#undef CHECK_RECOVERY
