#include "InspectionBlueprintLibrary.h"
#include "SearchableComponent.h"
#include "InspectionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
bool UInspectionBlueprintLibrary::IsInteractionAvailable(AActor* Target)
{
    if (!IsValid(Target)) return false;
    const auto* Searchable = Target->FindComponentByClass<USearchableComponent>();
    return !Searchable || !Searchable->IsComplete();
}
bool UInspectionBlueprintLibrary::IsInteractionInputAllowed(AActor* Interactor)
{
    APlayerController* PC = Cast<APlayerController>(Interactor);
    if (const auto* Pawn = Cast<APawn>(Interactor)) PC = Cast<APlayerController>(Pawn->GetController());
    if (!PC || !PC->GetLocalPlayer()) return false;
    const auto* Session = PC->GetLocalPlayer()->GetSubsystem<UInspectionSubsystem>();
    return !Session || !Session->IsOpen();
}
void UInspectionBlueprintLibrary::OpenInspection(AActor* Target, AActor* Interactor)
{
    if (IsValid(Target))
        if (auto* Source = Target->FindComponentByClass<USearchableComponent>()) Source->OpenInspection(Interactor);
}
