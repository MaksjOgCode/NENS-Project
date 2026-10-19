#include "ComputerRecoveryWidget.h"
#include "ComputerRecoverySubsystem.h"
#include "InteractionSessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Font.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

void UComputerRecoveryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_LoadCards")))) B->OnClicked.AddDynamic(this,&ThisClass::LoadCards);
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Restore")))) B->OnClicked.AddDynamic(this,&ThisClass::RestoreComputer);
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnClose")))) B->OnClicked.AddDynamic(this,&UMinigameWidget::RequestClose);
}
void UComputerRecoveryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Progress=UComputerRecoverySubsystem::From(this);
    if (Progress.IsValid()) Progress->OnProgressChanged.AddUniqueDynamic(this,&ThisClass::Refresh);
    Refresh();
}
void UComputerRecoveryWidget::LoadCards() { if (IsInteractive() && Progress.IsValid()) Progress->LoadProgramCards(); }
void UComputerRecoveryWidget::RestoreComputer() { if (IsInteractive() && Progress.IsValid()) Progress->BeginRestoration(); }
void UComputerRecoveryWidget::Refresh()
{
    auto* P=Progress.Get(); if (!P) return;
    auto SetText=[this](FName Name,const FText& Copy) { if (auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(Name))) T->SetText(Copy); };
    const bool Ready[]={P->IsSystemReady(EComputerSystem::Power),P->IsSystemReady(EComputerSystem::Fuel),P->IsSystemReady(EComputerSystem::Display),P->IsSystemReady(EComputerSystem::Arithmetic),P->GetRequiredCardCount()>0 && P->GetLoadedCardCount()==P->GetRequiredCardCount()};
    const FName Names[]={TEXT("Txt_PowerStatus"),TEXT("Txt_FuelStatus"),TEXT("Txt_DisplayStatus"),TEXT("Txt_ArithmeticStatus"),TEXT("Txt_ProgramStatus")};
    for (int32 I=0;I<5;++I)
    {
        if (Ready[I] && !LastReady[I]) ReadyOpacity[I]=.35f;
        LastReady[I]=Ready[I];
        if (I<4) SetText(Names[I],Ready[I] ? NSLOCTEXT("Recovery","Ready","READY") : NSLOCTEXT("Recovery","Offline","OFFLINE"));
        if (auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(Names[I]))) T->SetColorAndOpacity(FSlateColor(Ready[I] ? FLinearColor(.96f,.93f,.84f,1) : FLinearColor(.48f,.48f,.46f,1)));
    }
    SetText(TEXT("Txt_ProgramStatus"),FText::Format(NSLOCTEXT("Recovery","Loaded","{0} / {1} LOADED"),P->GetLoadedCardCount(),P->GetRequiredCardCount()));
    SetText(TEXT("Txt_ProgramHint"),FText::Format(NSLOCTEXT("Recovery","Bag","Punch cards in your bag: {0}\nReady to load: {1}"),P->GetInventoryCount(TEXT("PunchCard")),P->GetLoadableCardCount()));
    SetText(TEXT("Txt_NextStep"),P->GetNextStep());
    SetText(TEXT("Txt_State"),P->IsRestored() ? NSLOCTEXT("Recovery","StateOnline","COMPUTER ONLINE")
        : P->IsRestoring() ? NSLOCTEXT("Recovery","StateWorking","RECOVERING...")
        : P->CanRestore() ? NSLOCTEXT("Recovery","StateReady","RECOVERY READY")
        : P->GetReadySystemCount()==P->RequiredSystemCount ? NSLOCTEXT("Recovery","StateAwaitingCards","AWAITING PROGRAM") : NSLOCTEXT("Recovery","StateOffline","SYSTEMS OFFLINE"));
    SetText(TEXT("Txt_ProgressLabel"),P->IsRestoring() || P->IsRestored() ? NSLOCTEXT("Recovery","RunningLabel","RECOVERY PROGRAM") : NSLOCTEXT("Recovery","ReadinessLabel","SYSTEM READINESS"));
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_LoadCards")))) B->SetIsEnabled(P->GetLoadableCardCount()>0 && !P->IsRestoring() && !P->IsRestored());
    if (auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Restore")))) B->SetIsEnabled(P->CanRestore());
    if (auto* Image=Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Computer")))) Image->SetColorAndOpacity(P->IsRestored() ? FLinearColor::White : FLinearColor(.65f,.65f,.65f,1));
}
void UComputerRecoveryWidget::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime);
    auto* P=Progress.Get(); if (!P) return;
    const float Target=P->IsRestoring() || P->IsRestored() ? P->GetRestoreProgress() : (P->GetReadySystemCount()+float(P->GetLoadedCardCount())/FMath::Max(1,P->GetRequiredCardCount()))/(P->RequiredSystemCount+1.f);
    DisplayedProgress=FMath::FInterpTo(DisplayedProgress,Target,DeltaTime,8.f);
    if (auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("RecoveryProgress")))) Bar->SetPercent(DisplayedProgress);
    FNumberFormattingOptions NumberOptions; NumberOptions.SetMaximumFractionalDigits(0);
    if (auto* Copy=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_ProgressValue")))) Copy->SetText(FText::AsPercent(Target,&NumberOptions));
    const FName Names[]={TEXT("Txt_PowerStatus"),TEXT("Txt_FuelStatus"),TEXT("Txt_DisplayStatus"),TEXT("Txt_ArithmeticStatus"),TEXT("Txt_ProgramStatus")};
    for (int32 I=0;I<5;++I)
    {
        ReadyOpacity[I]=FMath::Min(1.f,ReadyOpacity[I]+DeltaTime*2);
        if (auto* T=WidgetTree->FindWidget(Names[I])) T->SetRenderOpacity(ReadyOpacity[I]);
    }
}
void UComputerRecoveryWidget::NativeDestruct()
{
    if (Progress.IsValid()) Progress->OnProgressChanged.RemoveDynamic(this,&ThisClass::Refresh);
    Progress.Reset(); Super::NativeDestruct();
}

URecoveryObjectiveWidget::URecoveryObjectiveWidget(const FObjectInitializer& Init):Super(Init)
{
    static ConstructorHelpers::FObjectFinder<UFont> F(TEXT("/Game/NENS/UI/Fonts/F_IMFellEnglish.F_IMFellEnglish")); Font=F.Object;
}
void URecoveryObjectiveWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetVisibility(ESlateVisibility::HitTestInvisible); SetRenderOpacity(0);
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"));
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Root;
    Frame=WidgetTree->ConstructWidget<USizeBox>(); Frame->SetWidthOverride(420);
    auto* PanelSlot=Root->AddChildToCanvas(Frame); PanelSlot->SetAnchors(FAnchors(1,0)); PanelSlot->SetAlignment(FVector2D(1,0)); PanelSlot->SetPosition(FVector2D(-24,24)); PanelSlot->SetAutoSize(true);
    auto* Panel=WidgetTree->ConstructWidget<UBorder>(); Frame->AddChild(Panel);
    Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.012f,.013f,.014f,.8f),5.f)); Panel->SetPadding(FMargin(18,14));
    auto* Content=WidgetTree->ConstructWidget<UVerticalBox>(); Panel->AddChild(Content);
    auto Add=[&](FName Name,const FText& Copy,int32 Size,FLinearColor Color)
    {
        auto* T=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
        T->SetFont(FSlateFontInfo(Font,Size,TEXT("Regular"))); T->SetText(Copy); T->SetColorAndOpacity(FSlateColor(Color)); T->SetAutoWrapText(true);
        Content->AddChildToVerticalBox(T)->SetPadding(FMargin(0,0,0,6)); return T;
    };
    Add(TEXT("RecoveryObjectiveTitle"),NSLOCTEXT("Recovery","HUDTitle","RESTORE THE MAIN COMPUTER"),17,FLinearColor(.8f,.78f,.72f,1));
    Summary=Add(TEXT("RecoveryObjectiveSummary"),FText(),19,FLinearColor(.94f,.92f,.86f,1));
    Notice=Add(TEXT("RecoveryObjectiveNotice"),FText(),18,FLinearColor(.62f,.62f,.58f,1));
}
void URecoveryObjectiveWidget::NativeConstruct()
{
    Super::NativeConstruct(); Progress=UComputerRecoverySubsystem::From(this);
    if (Progress.IsValid()) Progress->OnProgressChanged.AddUniqueDynamic(this,&ThisClass::Changed);
    Changed();
}
void URecoveryObjectiveWidget::Changed()
{
    auto* P=Progress.Get(); if (!P || !Summary || !Notice) return;
    Summary->SetText(P->IsRestored() ? NSLOCTEXT("Recovery","HUDOnline","COMPUTER ONLINE") : P->IsRestoring() ? NSLOCTEXT("Recovery","HUDWorking","Recovery program running...")
        : FText::Format(NSLOCTEXT("Recovery","HUDProgressFour","Systems: {0} / {4}    Program: {1} / {2}\nPunch cards in your bag: {3}"),P->GetReadySystemCount(),P->GetLoadedCardCount(),P->GetRequiredCardCount(),P->GetInventoryCount(TEXT("PunchCard")),P->RequiredSystemCount));
    Notice->SetText(P->IsRestoring() || P->IsRestored() ? FText::GetEmpty() : P->GetLatestNotice()); NoticeAge=0;
    Notice->SetVisibility(Notice->GetText().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
void URecoveryObjectiveWidget::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime); NoticeAge+=DeltaTime;
    if (Frame) Frame->SetWidthOverride(FMath::Min(420.f,FMath::Max(1.f,float(G.GetLocalSize().X)-48.f)));
    bool Modal=false;
    if (auto* PC=GetOwningPlayer()) if (auto* LP=PC->GetLocalPlayer()) Modal=LP->GetSubsystem<UInteractionSessionSubsystem>()->IsOpen();
    SetRenderOpacity(FMath::FInterpTo(GetRenderOpacity(),Modal ? 0.f : 1.f,DeltaTime,12.f));
    if (Notice) Notice->SetRenderOpacity(FMath::Clamp((5.f-NoticeAge)/.5f,0.f,1.f));
}
void URecoveryObjectiveWidget::NativeDestruct()
{
    if (Progress.IsValid()) Progress->OnProgressChanged.RemoveDynamic(this,&ThisClass::Changed);
    Progress.Reset(); Super::NativeDestruct();
}
