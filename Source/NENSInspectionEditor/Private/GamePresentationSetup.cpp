#include "InspectionEditorLibrary.h"
#include "GamePresentationData.h"
#include "GameExperienceSubsystem.h"
#include "EndGameWidget.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintEditorLibrary.h"
#include "WidgetBlueprint.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace InspectionSetup { bool SuccessJSON(const FString&); bool Save(UObject*); FString Fail(const FString&); }
#define CHECK_PRESENTATION(C,M) if (!(C)) return InspectionSetup::Fail(M)
FString UInspectionEditorLibrary::ConfigureGamePresentation()
{
    using namespace InspectionSetup;
    auto* Menu=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/NENS/UI/MainMenu/WBP_MainMenu.WBP_MainMenu"));
    auto* End=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/NENS/UI/Ending/WBP_EndGame.WBP_EndGame"));
    auto* GI=LoadObject<UBlueprint>(nullptr,TEXT("/Game/NENS/System/BP_NENSGameInstance.BP_NENSGameInstance"));
    auto* Data=LoadObject<UGamePresentationData>(nullptr,TEXT("/Game/NENS/System/DA_GamePresentation.DA_GamePresentation"));
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    CHECK_PRESENTATION(Menu && End && GI && Data && Font,TEXT("Presentation assets missing"));
    UWidgetTree* Tree=End->WidgetTree;
    if (!Tree->FindWidget(TEXT("ScaleBox_Minigame")))
    {
        auto* Root=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Root")); Tree->RootWidget=Root;
        auto* Black=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Image_Background"));
        FSlateBrush Brush; Brush.DrawAs=ESlateBrushDrawType::Image; Brush.TintColor=FSlateColor(FLinearColor(.008f,.009f,.011f,1)); Black->SetBrush(Brush);
        auto* BlackSlot=Root->AddChildToCanvas(Black); BlackSlot->SetAnchors(FAnchors(0,0,1,1)); BlackSlot->SetOffsets(FMargin(0)); BlackSlot->SetZOrder(-20);
        auto* Fit=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("ScaleBox_Minigame")); Fit->SetStretch(EStretch::ScaleToFit);
        auto* FitSlot=Root->AddChildToCanvas(Fit); FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(32));
        auto* Size=Tree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(1560); Size->SetHeightOverride(1000);
        auto* Align=CastChecked<UScaleBoxSlot>(Fit->AddChild(Size)); Align->SetHorizontalAlignment(HAlign_Center); Align->SetVerticalAlignment(VAlign_Center);
        auto* Scene=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Canvas_Minigame")); Size->AddChild(Scene);
        auto Place=[](UCanvasPanel* C,UWidget* W,FVector2D P,FVector2D S) { auto* Slot=C->AddChildToCanvas(W); Slot->SetPosition(P); Slot->SetSize(S); };
        auto Text=[&](FName Name,const TCHAR* Copy,int32 Points,float Y,float Height,FLinearColor Color=FLinearColor(.9f,.88f,.82f,1))
        {
            auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name); T->SetText(FText::FromString(Copy));
            T->SetFont(FSlateFontInfo(Font,Points,TEXT("Regular"))); T->SetColorAndOpacity(FSlateColor(Color)); T->SetJustification(ETextJustify::Center);
            T->SetAutoWrapText(true); T->SetVisibility(ESlateVisibility::HitTestInvisible); Place(Scene,T,{180,Y},{1200,Height});
        };
        Text(TEXT("Txt_Eyebrow"),TEXT("ALL SYSTEMS RESTORED"),22,180,44,FLinearColor(.55f,.55f,.5f,1));
        Text(TEXT("TxtTitle"),TEXT("The end"),76,270,124);
        Text(TEXT("Txt_Thanks"),TEXT("Thank you for playing!"),42,416,90);
        Text(TEXT("Txt_Story"),TEXT("The main computer is back online."),28,530,64,FLinearColor(.65f,.64f,.6f,1));
        FButtonStyle Style;
        Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.035f,.034f,1),4,FLinearColor(.35f,.34f,.3f,1),1));
        Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.12f,.12f,.11f,1),4,FLinearColor(.75f,.72f,.63f,1),1));
        Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1),4));
        for (int32 I=0;I<2;++I)
        {
            auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),I==0 ? TEXT("Btn_MainMenu") : TEXT("Btn_Quit")); B->SetStyle(Style);
            Place(Scene,B,{390.+I*420,690},{360,80});
            auto* Label=Tree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(I==0 ? TEXT("Main menu") : TEXT("Quit game")));
            Label->SetFont(FSlateFontInfo(Font,30,TEXT("Regular"))); Label->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1))); B->AddChild(Label);
        }
        Tree->ForEachWidget([End](UWidget* W){End->WidgetVariableNameToGuidMap.FindOrAdd(W->GetFName())=FGuid::NewGuid();});
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(End);
    CHECK_PRESENTATION(SuccessJSON(CompileAndSave(End,false)),TEXT("Ending compile failed"));
    Data->EndingWidget=End->GeneratedClass;
    CHECK_PRESENTATION(Data->Validate(),TEXT("Presentation definition invalid"));
    CastChecked<UGameExperienceInstance>(GI->GeneratedClass->GetDefaultObject())->Presentation=Data;
    FBlueprintEditorUtils::MarkBlueprintAsModified(GI);
    CHECK_PRESENTATION(SuccessJSON(CompileAndSave(GI,false)),TEXT("Game instance compile failed"));
    CastChecked<UGameExperienceInstance>(GI->GeneratedClass->GetDefaultObject())->Presentation=Data;
    if (Menu->ParentClass!=UMainMenuPresentationWidget::StaticClass())
    {
        CHECK_PRESENTATION(Menu->ParentClass==UUserWidget::StaticClass(),TEXT("Main menu already has a custom parent"));
        UBlueprintEditorLibrary::ReparentBlueprint(Menu,UMainMenuPresentationWidget::StaticClass());
    }
    TArray<UEdGraph*> Graphs; Menu->GetAllGraphs(Graphs);
    for (auto* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Event=Cast<UK2Node_ComponentBoundEvent>(Node))
            if (Event->ComponentPropertyName==TEXT("Btn_Play") || Event->ComponentPropertyName==TEXT("Btn_Quit"))
            {
                if (auto* Out=Event->FindPin(TEXT("then"))) Out->BreakAllPinLinks();
                Event->NodeComment=TEXT("GamePresentation: handled by native adapter with fade and new-run reset");
            }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Menu);
    CHECK_PRESENTATION(SuccessJSON(CompileAndSave(Menu,false)),TEXT("Main menu compile failed"));
    for (UObject* Object : {static_cast<UObject*>(End),static_cast<UObject*>(GI),static_cast<UObject*>(Data),static_cast<UObject*>(Menu)}) CHECK_PRESENTATION(Save(Object),TEXT("Presentation save failed"));
    return TEXT("{\"success\":true,\"blueprints\":3}");
}
#undef CHECK_PRESENTATION
