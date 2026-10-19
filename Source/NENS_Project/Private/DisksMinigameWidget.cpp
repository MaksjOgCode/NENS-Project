#include "DisksMinigameWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/UnrealType.h"

namespace Disks
{
const FName Rings[] = {TEXT("Ring_Inner"),TEXT("Ring_Middle"),TEXT("Ring_Outer")};
}
void UDisksMinigameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    Pages=Cast<UWidgetSwitcher>(WidgetTree->FindWidget(TEXT("ScenePages")));
    if (auto* Button=Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Cabinet")))) Button->OnClicked.AddDynamic(this,&ThisClass::OpenCabinet);
    if (auto* Button=Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnClose")))) Button->OnClicked.AddDynamic(this,&ThisClass::ExitPuzzle);
}
void UDisksMinigameWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // The old developer level still opens directly onto its puzzle.
    if (Pages) Pages->SetActiveWidgetIndex(IsManagedSession() ? 0 : 1);
    SetPageCopy();
}
bool UDisksMinigameWidget::IsShowingInspection() const { return Pages && Pages->GetActiveWidgetIndex()==0; }
bool UDisksMinigameWidget::IsInteractive() const { return Super::IsInteractive() && StageTransition==0; }
void UDisksMinigameWidget::SetPageCopy()
{
    if (auto* Title=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TxtTitle"))))
        Title->SetText(IsShowingInspection() ? NSLOCTEXT("Disks","Cabinet","Bedside cabinet") : NSLOCTEXT("Disks","Puzzle","Restore the image"));
    if (auto* Controls=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_DisksControls"))))
        Controls->SetText(IsShowingInspection() ? NSLOCTEXT("Disks","InspectControls","Left click — Inspect the cabinet\nEsc — Exit")
            : NSLOCTEXT("Disks","PuzzleControls","Hold left click — Turn\nEsc — Exit"));
}
void UDisksMinigameWidget::OpenCabinet()
{
    if (!IsInteractive() || !IsShowingInspection()) return;
    StageTransition=1; StageAge=0;
}
void UDisksMinigameWidget::StopTurning()
{
    if (auto* P=FindFProperty<FIntProperty>(GetClass(),TEXT("ActiveRing"))) P->SetPropertyValue_InContainer(this,-1);
}
void UDisksMinigameWidget::ExitPuzzle() { StopTurning(); RequestClose(); }
bool UDisksMinigameWidget::CanReportSuccess() const
{
    if (IsShowingInspection() || StageTransition) return false;
    if (auto* Active=FindFProperty<FIntProperty>(GetClass(),TEXT("ActiveRing")))
        if (Active->GetPropertyValue_InContainer(this)>=0) return false;
    for (FName Name : Disks::Rings)
    {
        auto* Ring=Cast<UImage>(WidgetTree->FindWidget(Name));
        if (!Ring || FMath::Abs(FMath::FindDeltaAngleDegrees(Ring->GetRenderTransformAngle(),0.f))>=10.f) return false;
    }
    return true;
}
void UDisksMinigameWidget::CompleteDiskPuzzle()
{
    if (!IsInteractive() || HasReportedSuccess() || !CanReportSuccess()) return;
    StopTurning();
    for (int32 I=0;I<3;++I) if (auto* Ring=Cast<UImage>(WidgetTree->FindWidget(Disks::Rings[I])))
        SnapFrom[I]=FMath::FindDeltaAngleDegrees(0.f,Ring->GetRenderTransformAngle());
    ReportSuccess();
    if (HasReportedSuccess())
        if (auto* Status=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TxtStatus")))) Status->SetText(NSLOCTEXT("Disks","Solved","Image restored"));
}
void UDisksMinigameWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);
    if (IsClosing())
    {
        StopTurning();
        if (auto* Button=WidgetTree->FindWidget(TEXT("Btn_Disks")))
            if (Button->HasMouseCapture() && FSlateApplication::IsInitialized()) FSlateApplication::Get().ReleaseAllPointerCapture();
        return;
    }
    if (StageTransition && Pages)
    {
        StageAge+=DeltaTime;
        const float T=FMath::Clamp(StageAge/FMath::Max(.05f,StageFadeSeconds),0.f,1.f), Ease=T*T*(3-2*T);
        Pages->SetRenderOpacity(StageTransition==1 ? 1-Ease : Ease);
        for (FName Name : {TEXT("TxtTitle"),TEXT("TxtStatus"),TEXT("Text_DisksControls")})
            if (auto* Copy=WidgetTree->FindWidget(Name)) Copy->SetRenderOpacity(Pages->GetRenderOpacity());
        if (T>=1)
        {
            if (StageTransition==1) { Pages->SetActiveWidgetIndex(1); SetPageCopy(); StageTransition=2; StageAge=0; }
            else
            {
                StageTransition=0; Pages->SetRenderOpacity(1);
                if (auto* PC=GetOwningPlayer()) SetUserFocus(PC);
            }
        }
    }
    if (HasReportedSuccess())
    {
        SuccessAge+=DeltaTime;
        const float T=FMath::Clamp(SuccessAge/.18f,0.f,1.f), Ease=T*T*(3-2*T);
        for (int32 I=0;I<3;++I) if (auto* Ring=Cast<UImage>(WidgetTree->FindWidget(Disks::Rings[I]))) Ring->SetRenderTransformAngle(SnapFrom[I]*(1-Ease));
        // Preserve the original 1.5-second OnDisksSolved delegate before closing.
        if (SuccessAge>=1.65f) RequestClose();
    }
}
FReply UDisksMinigameWidget::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (!IsInteractive()) return FReply::Handled();
    // Switching pages can retire the previous focus path. Keep Escape on this modal,
    // including when a non-focusable disk button captures a held mouse press.
    if (auto* PC=GetOwningPlayer()) SetUserFocus(PC);
    if (!IsShowingInspection() && Event.GetEffectingButton()==EKeys::LeftMouseButton)
    {
        PressPosition=Event.GetScreenSpacePosition();
        if (auto* Board=WidgetTree->FindWidget(TEXT("Board")))
        {
            const auto& G=Board->GetCachedGeometry();
            const auto Local=G.AbsoluteToLocal(PressPosition);
            // Ignore the square's corners; only the actual disk is interactive.
            if (Local.X>=0 && Local.Y>=0 && Local.X<=1000 && Local.Y<=1000 && (Local-FVector2D(500,500)).Size()>500)
                return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewMouseButtonDown(Geometry,Event);
}
FReply UDisksMinigameWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Escape) StopTurning();
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void UDisksMinigameWidget::NativeDestruct() { StopTurning(); Super::NativeDestruct(); }
