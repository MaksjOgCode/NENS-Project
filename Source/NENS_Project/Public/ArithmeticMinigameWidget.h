#pragma once
#include "CoreMinimal.h"
#include "MinigameWidget.h"
#include "Components/EditableTextBox.h"
#include "ArithmeticMinigameWidget.generated.h"
class UArithmeticPuzzleData;
class UImage;
class UTextBlock;
class UButton;

UCLASS()
class NENS_PROJECT_API UArithmeticMinigameWidget : public UMinigameWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Arithmetic") TObjectPtr<UArithmeticPuzzleData> Puzzle;
    UFUNCTION(BlueprintPure,Category="Arithmetic") int32 GetDisplayedFrame() const { return Frame; }
    UFUNCTION(BlueprintPure,Category="Arithmetic") bool IsAnswerReady() const;
    UFUNCTION(BlueprintCallable,Category="Arithmetic") void SubmitAnswer();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry&,float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual bool CanReportSuccess() const override { return bCorrect && IsAnswerReady(); }
private:
    UPROPERTY(Transient) TObjectPtr<UImage> CurrentImage;
    UPROPERTY(Transient) TObjectPtr<UImage> IncomingImage;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> AnswerBox;
    UPROPERTY(Transient) TObjectPtr<UButton> SubmitButton;
    UPROPERTY(Transient) TObjectPtr<UWidget> InputPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Stage;
    int32 Frame=0;
    float StageAge=0,InputAge=0,FeedbackAge=0,SuccessAge=0;
    bool bBlending=false,bFocused=false,bCorrect=false,bValid=false;
    void SetFrame(UImage* Image,int32 Index);
    void ShowFeedback(const FText&,bool bSuccess);
    UFUNCTION() void AnswerChanged(const FText& Text);
    UFUNCTION() void AnswerCommitted(const FText& Text,ETextCommit::Type Method);
};
