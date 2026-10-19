#include "InspectionEditorLibrary.h"
#include "ArithmeticMinigameWidget.h"
#include "ArithmeticPuzzleData.h"
#include "MinigameComponent.h"
#include "ComputerRecoveryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UnrealType.h"

namespace InspectionSetup { bool SuccessJSON(const FString&); bool Save(UObject*); FString Fail(const FString&); }
namespace ArithmeticSetup
{
UCanvasPanelSlot* Place(UCanvasPanel* Parent,UWidget* W,FVector2D P,FVector2D S)
{
    auto* Slot=Parent->AddChildToCanvas(W); Slot->SetPosition(P); Slot->SetSize(S); return Slot;
}
UTextBlock* Text(UWidgetTree* Tree,UCanvasPanel* Parent,UFont* Font,FName Name,const TCHAR* Copy,int32 Size,FVector2D P,FVector2D S)
{
    auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name); T->SetText(FText::FromString(Copy));
    T->SetFont(FSlateFontInfo(Font,Size,TEXT("Regular"))); T->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1)));
    T->SetAutoWrapText(true); T->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Parent,T,P,S); return T;
}
UButton* Button(UWidgetTree* Tree,UCanvasPanel* Parent,UFont* Font,FName Name,const TCHAR* Copy,FVector2D P,FVector2D S)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.04f,.04f,.04f,1),4,FLinearColor(.35f,.34f,.3f,1),1));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.14f,.14f,.13f,1),4,FLinearColor(.7f,.68f,.6f,1),1));
    Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1),4));
    Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.03f,.03f,.03f,1),4));
    auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name); B->SetStyle(Style); Place(Parent,B,P,S);
    if (auto* Focus=FindFProperty<FBoolProperty>(UButton::StaticClass(),TEXT("IsFocusable"))) Focus->SetPropertyValue_InContainer(B,false);
    auto* Label=Tree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Copy)); Label->SetFont(FSlateFontInfo(Font,26,TEXT("Regular")));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); B->AddChild(Label); return B;
}
UActorComponent* Override(UBlueprint* BP,UBlueprint* Base,FName Name)
{
    auto* Node=Base->SimpleConstructionScript->FindSCSNode(Name); if (!Node) return nullptr;
    auto* Handler=BP->GetInheritableComponentHandler(true); FComponentKey Key(Node);
    auto* Template=Handler->GetOverridenComponentTemplate(Key);
    return Template ? Template : Handler->CreateOverridenComponentTemplate(Key);
}
}
#define REQUIRE_ARITHMETIC(C,M) if (!(C)) return InspectionSetup::Fail(M)
FString UInspectionEditorLibrary::ConfigureArithmeticMinigame()
{
    using namespace ArithmeticSetup; using namespace InspectionSetup;
    auto* Widget=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/NENS/UI/Arithmetic/WBP_ArithmeticComputer.WBP_ArithmeticComputer"));
    auto* Data=LoadObject<UArithmeticPuzzleData>(nullptr,TEXT("/Game/NENS/UI/Arithmetic/DA_ArithmeticComputer.DA_ArithmeticComputer"));
    auto* Computer=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Environment/Interior/Blueprints/BP_ComputerCabinet.BP_ComputerCabinet"));
    auto* Station=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_ElectroStation.BP_Props_ElectroStation"));
    auto* Base=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase.BP_InteractableBase"));
    auto* Sprite=LoadObject<UPaperSprite>(nullptr,TEXT("/Game/NENS/Environment/Interior/Props/Sprites/SPR_Props_Electro_Station.SPR_Props_Electro_Station"));
    auto* Wires=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/Minigames/Wires/WBP_WiresMiniGame.WBP_WiresMiniGame"));
    auto* Recovery=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/NENS/UI/Recovery/WBP_ComputerRecovery.WBP_ComputerRecovery"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    REQUIRE_ARITHMETIC(Widget && Data && Data->Validate() && Computer && Station && Base && Sprite && Wires && Recovery && Font,TEXT("Arithmetic assets missing"));
    UWidgetTree* Tree=Widget->WidgetTree;
    if (!Tree->FindWidget(TEXT("ScaleBox_Minigame")))
    {
        auto* Root=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Root")); Tree->RootWidget=Root;
        auto* Fit=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ScaleBox_Minigame")); Fit->SetStretch(EStretch::ScaleToFit);
        auto* Slot=Root->AddChildToCanvas(Fit); Slot->SetAnchors(FAnchors(0,0,1,1)); Slot->SetOffsets(FMargin(32));
        auto* Size=Tree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1920); Size->SetHeightOverride(1280);
        auto* FitSlot=CastChecked<UScaleBoxSlot>(Fit->AddChild(Size)); FitSlot->SetHorizontalAlignment(HAlign_Center); FitSlot->SetVerticalAlignment(VAlign_Center);
        auto* Scene=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Minigame")); Size->AddChild(Scene);
        Text(Tree,Scene,Font,TEXT("Txt_Eyebrow"),TEXT("COMPUTER DIAGNOSTICS"),20,{80,22},{1700,32});
        Text(Tree,Scene,Font,TEXT("TxtTitle"),TEXT("Arithmetic check"),48,{80,62},{1700,70});
        Text(Tree,Scene,Font,TEXT("Txt_Description"),TEXT("Verify the computer's calculations."),26,{80,141},{340,90});
        Text(Tree,Scene,Font,TEXT("Txt_Stage"),TEXT("Reading equation 1 of 4..."),30,{80,310},{320,170});
        Text(Tree,Scene,Font,TEXT("Txt_Instructions"),TEXT("Study the equations.\nEnter the missing result."),26,{80,570},{320,150});
        Text(Tree,Scene,Font,TEXT("Text_ArithmeticControls"),TEXT("Type — Enter result\nEnter — Check answer\nEsc — Exit"),24,{80,820},{320,180});
        auto* Frame=Tree->ConstructWidget<UBorder>(); Frame->SetPadding(FMargin(2));
        Frame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.18f,.18f,.17f,1),3)); Place(Scene,Frame,{480,170},{1360,906});
        auto* Images=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Frames")); Frame->AddChild(Images);
        for (FName Name : {TEXT("Image_Current"),TEXT("Image_Incoming")})
        {
            auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Name); Image->SetVisibility(ESlateVisibility::HitTestInvisible);
            auto* ImageSlot=Images->AddChildToCanvas(Image); ImageSlot->SetAnchors(FAnchors(0,0,1,1)); ImageSlot->SetOffsets(FMargin(0));
        }
        auto* AnswerPanel=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Panel_Answer")); AnswerPanel->SetPadding(FMargin(0));
        AnswerPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.026f,.027f,.028f,.97f),4)); Place(Scene,AnswerPanel,{480,1096},{1360,104});
        auto* Row=Tree->ConstructWidget<UCanvasPanel>(); AnswerPanel->AddChild(Row);
        Text(Tree,Row,Font,TEXT("Txt_AnswerLabel"),TEXT("RESULT"),20,{24,37},{200,42});
        auto* Input=Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("Input_Answer"));
        FEditableTextBoxStyle InputStyle;
        InputStyle.SetFont(FSlateFontInfo(Font,30,TEXT("Regular"))); InputStyle.SetForegroundColor(FSlateColor(FLinearColor(.94f,.92f,.85f,1)));
        InputStyle.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(.015f,.015f,.015f,1),3,FLinearColor(.35f,.34f,.3f,1),1));
        InputStyle.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(.04f,.04f,.04f,1),3,FLinearColor(.6f,.58f,.51f,1),1));
        InputStyle.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(.03f,.03f,.03f,1),3,FLinearColor(.85f,.82f,.72f,1),1));
        InputStyle.SetPadding(FMargin(16,4)); Input->SetWidgetStyle(InputStyle); Input->SetHintText(FText::FromString(TEXT("Number"))); Input->SetJustification(ETextJustify::Center);
        Place(Row,Input,{240,16},{300,72});
        Button(Tree,Row,Font,TEXT("Btn_Submit"),TEXT("Check"),{570,24},{210,56});
        Text(Tree,Row,Font,TEXT("Txt_Feedback"),TEXT(""),24,{810,17},{520,80});
        Button(Tree,Scene,Font,TEXT("BtnClose"),TEXT("Exit"),{80,1144},{220,56});
        Tree->ForEachWidget([Widget](UWidget* W){ Widget->WidgetVariableNameToGuidMap.FindOrAdd(W->GetFName())=FGuid::NewGuid(); });
    }
    // Apply readability adjustments to existing instances as well.
    if (auto* Label=Cast<UTextBlock>(Tree->FindWidget(TEXT("Txt_AnswerLabel")))) Label->SetText(FText::FromString(TEXT("RESULT")));
    if (auto* Input=Cast<UEditableTextBox>(Tree->FindWidget(TEXT("Input_Answer"))))
    {
        Input->SetHintText(FText::FromString(TEXT("Number")));
        auto Style=Input->GetWidgetStyle(); Style.SetFont(FSlateFontInfo(Font,30,TEXT("Regular"))); Input->SetWidgetStyle(Style);
        auto* Slot=CastChecked<UCanvasPanelSlot>(Input->Slot); Slot->SetPosition({240,16}); Slot->SetSize({300,72});
    }
    CastChecked<UArithmeticMinigameWidget>(Widget->GeneratedClass->GetDefaultObject())->Puzzle=Data;
    FBlueprintEditorUtils::MarkBlueprintAsModified(Widget);
    REQUIRE_ARITHMETIC(SuccessJSON(CompileAndSave(Widget,false)),TEXT("Arithmetic widget compile failed"));
    // Set defaults after skeleton regeneration as well, before saving.
    CastChecked<UArithmeticMinigameWidget>(Widget->GeneratedClass->GetDefaultObject())->Puzzle=Data;
    auto* Visual=Cast<UPaperSpriteComponent>(Override(Station,Base,TEXT("Sprite_Visual")));
    REQUIRE_ARITHMETIC(Visual,TEXT("Station visual missing")); Visual->SetSprite(Sprite); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (auto* Ring=Cast<UStaticMeshComponent>(Override(Station,Base,TEXT("Ring_Visual")))) { Ring->SetRelativeScale3D({.9,.9,1}); Ring->SetRelativeLocation({0,0,-.5}); Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    for (auto* BP : {Station,Computer})
    {
        auto* Node=BP->SimpleConstructionScript->FindSCSNode(TEXT("Minigame"));
        auto* Game=Node ? Cast<UMinigameComponent>(Node->ComponentTemplate) : nullptr;
        REQUIRE_ARITHMETIC(Game,TEXT("Puzzle component missing"));
        Game->WidgetClass=BP==Station ? Wires->GeneratedClass : Widget->GeneratedClass;
        Game->RepairedSystem=BP==Station ? EComputerSystem::Power : EComputerSystem::Arithmetic;
        FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
        REQUIRE_ARITHMETIC(SuccessJSON(CompileAndSave(BP,false)),TEXT("Puzzle actor compile failed"));
    }
    // Preserve the authored terminal, compact its cards, then add the new system.
    UWidgetTree* RT=Recovery->WidgetTree; auto* RC=Cast<UCanvasPanel>(RT->FindWidget(TEXT("Canvas_Minigame")));
    REQUIRE_ARITHMETIC(RC,TEXT("Recovery canvas missing"));
    const TCHAR* Keys[]={TEXT("Power"),TEXT("Fuel"),TEXT("Display"),TEXT("Arithmetic"),TEXT("Program")};
    for (int32 I=0;I<5;++I)
    {
        const FName PanelName(*FString::Printf(TEXT("Panel_%s"),Keys[I])); auto* Panel=RT->FindWidget(PanelName);
        if (!Panel && I==3)
        {
            auto* Border=RT->ConstructWidget<UBorder>(UBorder::StaticClass(),PanelName); Border->SetPadding(FMargin(0));
            Border->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.023f,.024f,.025f,.94f),6,FLinearColor(.17f,.17f,.16f,1),1));
            auto* Row=RT->ConstructWidget<UCanvasPanel>(); Border->AddChild(Row); Panel=Border; Place(RC,Panel,{720,600},{760,118});
            Text(RT,Row,Font,TEXT("Txt_ArithmeticLabel"),TEXT("ARITHMETIC CHECK"),20,{24,18},{470,34});
            auto* Status=Text(RT,Row,Font,TEXT("Txt_ArithmeticStatus"),TEXT("OFFLINE"),24,{490,18},{246,36}); Status->SetJustification(ETextJustify::Right);
            Text(RT,Row,Font,TEXT("Txt_ArithmeticHint"),TEXT("Verify the result at the small computer."),24,{24,57},{712,60});
        }
        REQUIRE_ARITHMETIC(Panel,TEXT("Recovery card missing"));
        if (auto* Slot=Cast<UCanvasPanelSlot>(Panel->Slot)) { Slot->SetPosition({720,210.+I*130}); Slot->SetSize({760,118}); }
        if (auto* Hint=Cast<UTextBlock>(RT->FindWidget(FName(*FString::Printf(TEXT("Txt_%sHint"),Keys[I])))))
        {
            if (auto* Slot=Cast<UCanvasPanelSlot>(Hint->Slot)) { Slot->SetPosition({24,54}); Slot->SetSize({712,64}); }
            Hint->SetFont(FSlateFontInfo(Font,22,TEXT("Regular")));
        }
    }
    RT->ForEachWidget([Recovery](UWidget* W){ if (!Recovery->WidgetVariableNameToGuidMap.Contains(W->GetFName())) Recovery->WidgetVariableNameToGuidMap.Add(W->GetFName(),FGuid::NewGuid()); });
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Recovery);
    REQUIRE_ARITHMETIC(SuccessJSON(CompileAndSave(Recovery,false)),TEXT("Recovery presentation compile failed"));
    for (auto* BP : {static_cast<UBlueprint*>(Widget),Station,Computer,static_cast<UBlueprint*>(Recovery)}) REQUIRE_ARITHMETIC(Save(BP),TEXT("Could not save arithmetic Blueprint"));
    return TEXT("{\"success\":true,\"blueprints\":4}");
}
#undef REQUIRE_ARITHMETIC
