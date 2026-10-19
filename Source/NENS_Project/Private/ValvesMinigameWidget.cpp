#include "ValvesMinigameWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/NativeWidgetHost.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "UObject/UnrealType.h"
#include "Widgets/SLeafWidget.h"

class SValveIndicator : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SValveIndicator) {} SLATE_END_ARGS()
    void Construct(const FArguments&) {}
    void Update(float InAngle, float InTarget, bool InAligned)
    {
        Angle=InAngle; Target=InTarget; bAligned=InAligned; Invalidate(EInvalidateWidgetReason::Paint);
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(172,172); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out,
        int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        const FVector2f Center(G.GetLocalSize()*.5);
        const float D=FMath::Min(G.GetLocalSize().X,G.GetLocalSize().Y);
        const auto Tint=Style.GetColorAndOpacityTint();
        const FLinearColor White=FLinearColor(.96f,.94f,.87f,1)*Tint, Black=FLinearColor(.008f,.008f,.008f,1)*Tint;
        auto Dir=[](float Degrees) { float A=FMath::DegreesToRadians(Degrees); return FVector2f(FMath::Sin(A),-FMath::Cos(A)); };
        auto Lines=[&](const TArray<FVector2f>& Points, FLinearColor Color, float Thickness, int32 Z)
        { FSlateDrawElement::MakeLines(Out,Layer+Z,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Thickness); };
        const auto Aim=Dir(Angle), Side=FVector2f(-Aim.Y,Aim.X);
        const TArray<FVector2f> Shaft={Center+Aim*D*.07f,Center+Aim*D*.34f};
        const TArray<FVector2f> Head={Center+Aim*D*.25f+Side*D*.045f,Center+Aim*D*.35f,Center+Aim*D*.25f-Side*D*.045f};
        Lines(Shaft,Black,8,0); Lines(Head,Black,8,0); Lines(Shaft,White,4,1); Lines(Head,White,4,1);
        const auto Mark=Center+Dir(Target)*D*.46f;
        const float R=D*.045f;
        const TArray<FVector2f> Diamond={Mark+FVector2f(0,-R),Mark+FVector2f(R,0),Mark+FVector2f(0,R),Mark+FVector2f(-R,0),Mark+FVector2f(0,-R)};
        Lines(Diamond,Black,7,2); Lines(Diamond,White,3,3);
        if (bAligned) Lines({Mark-FVector2f(0,R*.5f),Mark+FVector2f(0,R*.5f)},White,6,4);
        return Layer+4;
    }
private:
    float Angle=0,Target=0;
    bool bAligned=false;
};

double UValveControlWidget::ReadAngle(FName Name) const
{
    if (auto* P=FindFProperty<FNumericProperty>(GetClass(),Name))
        return P->GetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(this));
    return 0;
}
bool UValveControlWidget::IsAligned() const
{
    // Do not accept a wheel which has merely passed through the goal while still rotating.
    return FMath::Abs(ReadAngle(TEXT("CurrAngle"))-ReadAngle(TEXT("TargetAngle")))<1.
        && FMath::Abs(FMath::FindDeltaAngleDegrees(ReadAngle(TEXT("CurrAngle")),ReadAngle(TEXT("TargetSolutionAngle"))))<1.;
}
void UValveControlWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (auto* Root=Cast<UCanvasPanel>(WidgetTree->RootWidget))
    {
        auto* Host=WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(),TEXT("ValveOrientation"));
        Indicator=SNew(SValveIndicator);
        Host->SetContent(Indicator.ToSharedRef()); Host->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* CanvasSlot=Root->AddChildToCanvas(Host);
        CanvasSlot->SetAnchors(FAnchors(0,0,1,1)); CanvasSlot->SetOffsets(FMargin(0)); CanvasSlot->SetZOrder(10);
    }
}
void UValveControlWidget::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime);
    if (Indicator) Indicator->Update(ReadAngle(TEXT("CurrAngle")),ReadAngle(TEXT("TargetSolutionAngle")),IsAligned());
}
void UValveControlWidget::NativeDestruct() { Indicator.Reset(); Super::NativeDestruct(); }

void UValvesMinigameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (auto* Exit=Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnClose")))) Exit->OnClicked.AddDynamic(this,&UMinigameWidget::RequestClose);
}
int32 UValvesMinigameWidget::AlignedCount() const
{
    int32 Count=0;
    for (FName Name : {TEXT("Valve_Left"),TEXT("Valve_Top"),TEXT("Valve_Right")})
        if (auto* Valve=Cast<UValveControlWidget>(WidgetTree->FindWidget(Name))) if (Valve->IsAligned()) ++Count;
    return Count;
}
bool UValvesMinigameWidget::CanReportSuccess() const { return AlignedCount()==3; }
void UValvesMinigameWidget::NativeTick(const FGeometry& G,float DeltaTime)
{
    Super::NativeTick(G,DeltaTime);
    if (IsClosing()) return;
    const int32 Count=HasReportedSuccess() ? 3 : AlignedCount();
    if (Count!=LastCount)
    {
        LastCount=Count;
        if (auto* Status=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TxtStatus"))))
            Status->SetText(HasReportedSuccess() ? NSLOCTEXT("Valves","Solved","Flow restored")
                : FText::Format(NSLOCTEXT("Valves","Aligned","Aligned: {0} / 3"),FText::AsNumber(Count)));
    }
    if (HasReportedSuccess())
    {
        SuccessAge+=DeltaTime;
        if (SuccessAge>=.9f) RequestClose();
    }
}

