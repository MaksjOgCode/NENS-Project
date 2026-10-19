#include "InspectionEditorLibrary.h"
#include "WiresMinigameWidget.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintEditorLibrary.h"
#include "WidgetBlueprint.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "WiresPresentation"

FString UInspectionEditorLibrary::StyleWiresMinigame()
{
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Minigames/Wires/WBP_WiresMiniGame.WBP_WiresMiniGame"));
    if (!BP || !BP->WidgetTree) return TEXT("{\"success\":false,\"error\":\"Wires widget missing\"}");
    UWidgetTree* Tree = BP->WidgetTree;
    auto* Root = Cast<UCanvasPanel>(Tree->RootWidget);
    auto* BoardSize = Cast<USizeBox>(Tree->FindWidget(TEXT("SizeBox_115")));
    auto* Title = Cast<UTextBlock>(Tree->FindWidget(TEXT("TxtTitle")));
    auto* Status = Cast<UTextBlock>(Tree->FindWidget(TEXT("TxtStatus")));
    auto* Close = Cast<UButton>(Tree->FindWidget(TEXT("BtnClose")));
    auto* CloseText = Cast<UTextBlock>(Tree->FindWidget(TEXT("TextBlock_137")));
    auto* Font = LoadObject<UFont>(nullptr, TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish"));
    if (!Root || !BoardSize || !Title || !Status || !Close || !CloseText || !Font)
        return TEXT("{\"success\":false,\"error\":\"Original wires layout or font changed\"}");
    // The source has opaque black padding. Crop only the displayed background; keep the
    // original 1280x720 board coordinate system so every connector/wire stays aligned.
    bool bCroppedBackground = false;
    if (auto* Background = Cast<UImage>(Tree->FindWidget(TEXT("Img_Bg"))))
        if (auto* BackgroundSlot = Cast<UCanvasPanelSlot>(Background->Slot))
            if (!static_cast<FBox2f>(Background->GetBrush().GetUVRegion()).bIsValid)
            {
                BP->Modify(); Background->Modify(); BackgroundSlot->Modify();
                FSlateBrush Brush = Background->GetBrush();
                Brush.SetUVRegion(FBox2f(FVector2f(.092f,0), FVector2f(.902f,1)));
                Background->SetBrush(Brush);
                BackgroundSlot->SetPosition(FVector2D(1280 * .092,0));
                BackgroundSlot->SetSize(FVector2D(1280 * .81,720));
                bCroppedBackground = true;
            }
    // Do not rebuild an artist's later changes when the setup script is run twice.
    if (Tree->FindWidget(TEXT("ScaleBox_Wires")))
    {
        if (bCroppedBackground)
        {
            FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
            return CompileAndSave(BP);
        }
        return TEXT("{\"success\":true,\"already_styled\":true}");
    }

    BP->Modify(); Tree->Modify();
    Title->RemoveFromParent(); BoardSize->RemoveFromParent(); Status->RemoveFromParent(); Close->RemoveFromParent();
    Root->ClearChildren();
    auto* Fit = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ScaleBox_Wires"));
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->SetStretchDirection(EStretchDirection::Both);
    auto* FitSlot = Root->AddChildToCanvas(Fit);
    FitSlot->SetAnchors(FAnchors(0,0,1,1)); FitSlot->SetOffsets(FMargin(32)); FitSlot->SetZOrder(0);
    auto* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_WiresPresentation"));
    Size->SetWidthOverride(1440); Size->SetHeightOverride(1000);
    auto* Center = CastChecked<UScaleBoxSlot>(Fit->AddChild(Size));
    Center->SetHorizontalAlignment(HAlign_Center); Center->SetVerticalAlignment(VAlign_Center);
    auto* Scene = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_WiresPresentation"));
    Size->AddChild(Scene);
    auto Place = [Scene](UWidget* Widget, FVector2D Position, FVector2D Dimensions)
    {
        auto* CanvasSlot = Scene->AddChildToCanvas(Widget);
        CanvasSlot->SetPosition(Position); CanvasSlot->SetSize(Dimensions);
    };
    Place(Title, FVector2D(80,24), FVector2D(1280,64));
    // All six buttons, textures and wire geometry scale together in their original coordinates.
    Place(BoardSize, FVector2D(80,104), FVector2D(1280,720));
    Place(Status, FVector2D(80,850), FVector2D(1280,48));
    auto* Controls = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_WiresControls"));
    Place(Controls, FVector2D(80,916), FVector2D(1040,64));
    Place(Close, FVector2D(1180,918), FVector2D(180,52));
    Controls->SetText(LOCTEXT("Controls", "Left click — Select a left connector, then its matching symbol on the right.\nEsc — Exit"));
    Controls->SetJustification(ETextJustify::Left);
    Controls->SetLineHeightPercentage(1.15f);
    Title->SetText(LOCTEXT("Title", "Connect the wires"));
    CloseText->SetText(LOCTEXT("Exit", "Exit"));
    for (const auto& Entry : {TPair<UTextBlock*,int32>(Title,44), {Status,26}, {Controls,22}, {CloseText,26}})
    {
        Entry.Key->SetFont(FSlateFontInfo(Font, Entry.Value, TEXT("Regular")));
        Entry.Key->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.88f,.82f,1)));
        Entry.Key->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    Controls->SetColorAndOpacity(FSlateColor(FLinearColor(.65f,.64f,.6f,1)));
    FButtonStyle ButtonStyle;
    ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.035f,.034f,1), 4.f, FLinearColor(.3f,.29f,.26f,1),1.f));
    ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.09f,.09f,.085f,1), 4.f, FLinearColor(.6f,.58f,.52f,1),1.f));
    ButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.02f,.02f,.02f,1), 4.f));
    ButtonStyle.SetNormalPadding(FMargin(12,4)); ButtonStyle.SetPressedPadding(FMargin(12,5,12,3));
    Close->SetStyle(ButtonStyle);
    for (FName Name : {TEXT("ScaleBox_Wires"), TEXT("SizeBox_WiresPresentation"), TEXT("Canvas_WiresPresentation"), TEXT("Text_WiresControls")})
        BP->WidgetVariableNameToGuidMap.Add(Name, FGuid::NewGuid());
    UBlueprintEditorLibrary::ReparentBlueprint(BP, UWiresMinigameWidget::StaticClass());
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return CompileAndSave(BP);
}

#undef LOCTEXT_NAMESPACE
