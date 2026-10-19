#include "ComputerRecoveryComponent.h"
#include "ComputerRecoveryWidget.h"
#include "InteractionSessionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
UComputerRecoveryComponent::UComputerRecoveryComponent() { PrimaryComponentTick.bCanEverTick=false; }
void UComputerRecoveryComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Progress=UComputerRecoverySubsystem::From(this))
        if (Progress->Configure(Definition))
        {
            Progress->OnComputerRestored.AddUniqueDynamic(this,&ThisClass::Restored);
            if (bShowObjective) Progress->ShowObjective(UGameplayStatics::GetPlayerController(this,0));
        }
}
void UComputerRecoveryComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Session=ActiveSession.Get()) Session->Close(ActiveWidget);
    if (auto* Progress=UComputerRecoverySubsystem::From(this)) Progress->OnComputerRestored.RemoveDynamic(this,&ThisClass::Restored);
    Super::EndPlay(Reason);
}
bool UComputerRecoveryComponent::OpenComputer(AActor* Interactor)
{
    if (ActiveWidget || !WidgetClass || !IsValid(Interactor)) return false;
    auto* PC=Cast<APlayerController>(Interactor);
    if (auto* Pawn=Cast<APawn>(Interactor)) PC=Cast<APlayerController>(Pawn->GetController());
    if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer()) return false;
    auto* Session=PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
    auto* Progress=UComputerRecoverySubsystem::From(this);
    if (!Session || Session->IsOpen() || !Progress || !Progress->Configure(Definition)) return false;
    auto* Widget=CreateWidget<UComputerRecoveryWidget>(PC,WidgetClass);
    if (!Widget) return false;
    Widget->InitializePresentationSession(FSimpleDelegate::CreateUObject(this,&ThisClass::CloseFromWidget,Widget));
    ActiveWidget=Widget; ActiveSession=Session;
    if (!Session->Open(Widget,GetOwner(),FSimpleDelegate::CreateUObject(this,&ThisClass::SessionClosed)))
    { ActiveWidget=nullptr; ActiveSession.Reset(); return false; }
    if (bShowObjective) Progress->ShowObjective(PC);
    return true;
}
void UComputerRecoveryComponent::CloseFromWidget(UComputerRecoveryWidget* Expected)
{
    if (ActiveWidget==Expected) if (auto* Session=ActiveSession.Get()) Session->Close(Expected);
}
void UComputerRecoveryComponent::SessionClosed() { ActiveWidget=nullptr; ActiveSession.Reset(); }
void UComputerRecoveryComponent::Restored() { OnRestored.Broadcast(); }
