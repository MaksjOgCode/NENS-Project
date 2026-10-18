#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InspectionBlueprintLibrary.generated.h"
UCLASS()
class NENS_PROJECT_API UInspectionBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Ordinary furniture without a searchable component keeps its existing behaviour. */
    UFUNCTION(BlueprintPure, Category="Inspection") static bool IsInteractionAvailable(AActor* Target);
    UFUNCTION(BlueprintPure, Category="Inspection") static bool IsInteractionInputAllowed(AActor* Interactor);
    UFUNCTION(BlueprintCallable, Category="Inspection") static void OpenInspection(AActor* Target, AActor* Interactor);
};
