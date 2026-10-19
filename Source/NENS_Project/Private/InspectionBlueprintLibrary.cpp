#include "InspectionBlueprintLibrary.h"
#include "SearchableComponent.h"
#include "InspectionSubsystem.h"
#include "InteractionSessionSubsystem.h"
#include "MinigameComponent.h"
#include "ComputerRecoveryComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
bool UInspectionBlueprintLibrary::IsInteractionAvailable(AActor* Target)
{
    if (!IsValid(Target)) return false;
    const auto* Searchable = Target->FindComponentByClass<USearchableComponent>();
    const auto* Minigame = Target->FindComponentByClass<UMinigameComponent>();
    return (!Searchable || !Searchable->IsComplete()) && (!Minigame || !Minigame->IsComplete());
}
bool UInspectionBlueprintLibrary::IsInteractionInputAllowed(AActor* Interactor)
{
    APlayerController* PC = Cast<APlayerController>(Interactor);
    if (const auto* Pawn = Cast<APawn>(Interactor)) PC = Cast<APlayerController>(Pawn->GetController());
    if (!PC || !PC->GetLocalPlayer()) return false;
    const auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInteractionSessionSubsystem>();
    return !Session || !Session->IsOpen();
}
void UInspectionBlueprintLibrary::OpenInspection(AActor* Target, AActor* Interactor)
{
    if (IsValid(Target))
        if (auto* Source = Target->FindComponentByClass<USearchableComponent>()) Source->OpenInspection(Interactor);
}
bool UInspectionBlueprintLibrary::OpenMinigame(AActor* Target, AActor* Interactor)
{
    if (IsValid(Target))
        if (auto* Minigame = Target->FindComponentByClass<UMinigameComponent>()) return Minigame->OpenMinigame(Interactor);
    return false;
}
bool UInspectionBlueprintLibrary::OpenMainComputer(AActor* Target,AActor* Interactor)
{
    if (IsValid(Target)) if (auto* Computer=Target->FindComponentByClass<UComputerRecoveryComponent>()) return Computer->OpenComputer(Interactor);
    return false;
}
