#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InspectionEditorLibrary.generated.h"

class UBlueprint;
class UInspectionSceneData;

/** Small editor-only bridge used by the reproducible inspection setup scripts. */
UCLASS()
class NENSINSPECTIONEDITOR_API UInspectionEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString ConfigureInspection();
    /** Connect an existing demo interactable to the shared inspection widget and scene data. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString ConfigureSearchableActor(UBlueprint* Blueprint, UInspectionSceneData* Scene);
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString ConfigureWiresMinigame();
    /** Fit the existing wires board to the viewport; preserve its connectors and Blueprint rules. */
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString StyleWiresMinigame();
    /** Adapt the existing valve board and existing PickMeBox asset to the shared modal interaction. */
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString ConfigureValvesMinigame();
    /** Style the original disks and add the room inspection stage and placeholder cabinet. */
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString ConfigureDisksMinigame();
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString ConfigureComputerRecovery();
    UFUNCTION(BlueprintCallable,Category="Interaction|Editor")
    static FString ConfigureArithmeticMinigame();
    /** Replace the matching decorative station/main-computer actors, keeping their visible transforms. */
    UFUNCTION(BlueprintCallable,Category="Interaction|Editor")
    static FString ConnectArithmeticLevel(const FString& AssetPath);
    /** Use the interaction sphere's center and place the Gosling ring between floor and walls. */
    UFUNCTION(BlueprintCallable, Category="Interaction|Editor")
    static FString FixInteractionGeometry();
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString ConfigureClockInspection();
    /** Prevent repeated prompt animations and retire completed wardrobe indicators. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString FixInspectionPrompt();
    /** Read-only graph, component-template, default-property and widget-tree audit. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString DumpBlueprint(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString DumpLevel(const FString& AssetPath);

    /** Compile first; save only if compilation succeeds. Returns JSON diagnostics. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString CompileAndSave(UBlueprint* Blueprint, bool bSave = true);
};
