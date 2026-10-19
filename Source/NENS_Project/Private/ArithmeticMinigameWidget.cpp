#include "ArithmeticMinigameWidget.h"
#include "ArithmeticPuzzleData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"

void UArithmeticMinigameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    CurrentImage=Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Current")));
    IncomingImage=Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_Incoming")));
    AnswerBox=Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("Input_Answer")));
    SubmitButton=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Submit")));
    InputPanel=WidgetTree->FindWidget(TEXT("Panel_Answer"));
    Feedback=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_Feedback")));
    Stage=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_Stage")));
    bValid=Puzzle && Puzzle->Validate() && CurrentImage && IncomingImage && AnswerBox && SubmitButton && InputPanel;
    if (auto* Close=Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnClose")))) Close->OnClicked.AddDynamic(this,&UMinigameWidget::RequestClose);
    if (SubmitButton) SubmitButton->OnClicked.AddDynamic(this,&ThisClass::SubmitAnswer);
    if (AnswerBox)
    {
        AnswerBox->OnTextChanged.AddDynamic(this,&ThisClass::AnswerChanged);
        AnswerBox->OnTextCommitted.AddDynamic(this,&ThisClass::AnswerCommitted);
        AnswerBox->SetClearKeyboardFocusOnCommit(false); AnswerBox->SetSelectAllTextWhenFocused(true); AnswerBox->SetSelectAllTextOnCommit(true);
    }
    if (InputPanel) { InputPanel->SetRenderOpacity(0); InputPanel->SetIsEnabled(false); }
    if (bValid) { SetFrame(CurrentImage,0); SetFrame(IncomingImage,1); IncomingImage->SetRenderOpacity(0); }
    else if (Stage) Stage->SetText(NSLOCTEXT("Arithmetic","InvalidSetup","The diagnostic test is unavailable."));
}
void UArithmeticMinigameWidget::SetFrame(UImage* Image,int32 Index)
{
    Image->SetBrushFromTexture(Puzzle->Frames[Index]);
    auto Brush=Image->GetBrush(); Brush.SetUVRegion(FBox2f(FVector2f(0,.12f),FVector2f(1,1))); Image->SetBrush(Brush);
}
bool UArithmeticMinigameWidget::IsAnswerReady() const
{
    return bValid && IsInteractive() && Frame==3 && !bBlending && InputAge>=Puzzle->InputFadeSeconds;
}
void UArithmeticMinigameWidget::NativeTick(const FGeometry& G,float Dt)
{
    Super::NativeTick(G,Dt);
    if (!bValid || !IsInteractive()) return;
    if (HasReportedSuccess()) { SuccessAge+=Dt; if (SuccessAge>=1.4f) RequestClose(); return; }
    if (Frame<3)
    {
        StageAge+=Dt;
        if (!bBlending && StageAge>=Puzzle->HoldSeconds) { bBlending=true; StageAge=0; SetFrame(IncomingImage,Frame+1); }
        if (bBlending)
        {
            const float T=FMath::Clamp(StageAge/Puzzle->BlendSeconds,0.f,1.f);
            IncomingImage->SetRenderOpacity(T*T*(3-2*T));
            if (T>=1) { ++Frame; SetFrame(CurrentImage,Frame); IncomingImage->SetRenderOpacity(0); bBlending=false; StageAge=0; }
        }
        if (Stage) Stage->SetText(FText::Format(NSLOCTEXT("Arithmetic","Revealing","Reading equation {0} of 4..."),Frame+1));
    }
    else
    {
        InputAge+=Dt;
        const float T=FMath::Clamp(InputAge/Puzzle->InputFadeSeconds,0.f,1.f); InputPanel->SetRenderOpacity(T*T*(3-2*T));
        if (Stage) Stage->SetText(NSLOCTEXT("Arithmetic","Ready","What is the missing result?"));
        if (!bFocused && IsAnswerReady()) { InputPanel->SetIsEnabled(true); AnswerBox->SetKeyboardFocus(); bFocused=true; }
    }
    FeedbackAge+=Dt;
    if (Feedback) Feedback->SetRenderOpacity(FMath::Clamp(FeedbackAge/.2f,0.f,1.f));
}
void UArithmeticMinigameWidget::ShowFeedback(const FText& Copy,bool bSuccess)
{
    if (Feedback) { Feedback->SetText(Copy); Feedback->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(.9f,.88f,.8f,1) : FLinearColor(.86f,.66f,.58f,1))); Feedback->SetRenderOpacity(0); FeedbackAge=0; }
}
void UArithmeticMinigameWidget::SubmitAnswer()
{
    if (!IsAnswerReady() || HasReportedSuccess()) return;
    bCorrect=Puzzle->IsCorrect(AnswerBox->GetText().ToString());
    if (!bCorrect)
    {
        ShowFeedback(NSLOCTEXT("Arithmetic","Wrong","Incorrect result.\nPlease try again."),false);
        AnswerBox->SetKeyboardFocus(); return;
    }
    ReportSuccess();
    if (!HasReportedSuccess()) { bCorrect=false; return; }
    ShowFeedback(NSLOCTEXT("Arithmetic","Correct","Calculation verified."),true);
    if (Feedback) Feedback->SetRenderOpacity(1);
    if (Stage) Stage->SetText(NSLOCTEXT("Arithmetic","Passed","ARITHMETIC CHECK PASSED"));
    AnswerBox->SetIsReadOnly(true); SubmitButton->SetIsEnabled(false);
}
void UArithmeticMinigameWidget::AnswerChanged(const FText&) { if (!HasReportedSuccess() && Feedback) Feedback->SetText(FText::GetEmpty()); }
void UArithmeticMinigameWidget::AnswerCommitted(const FText&,ETextCommit::Type Method) { if (Method==ETextCommit::OnEnter) SubmitAnswer(); }
FReply UArithmeticMinigameWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Enter) { SubmitAnswer(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(G,Event);
}
