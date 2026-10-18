#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InspectionEditorLibrary.generated.h"

class UBlueprint;

/** Small editor-only bridge used by the reproducible inspection setup scripts. */
UCLASS()
class NENSINSPECTIONEDITOR_API UInspectionEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString ConfigureInspection();
    /** Prevent repeated prompt animations and retire completed wardrobe indicators. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString FixInspectionPrompt();
    /** Read-only graph, component-template, default-property and widget-tree audit. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString DumpBlueprint(UBlueprint* Blueprint);

    /** Compile first; save only if compilation succeeds. Returns JSON diagnostics. */
    UFUNCTION(BlueprintCallable, Category="Inspection|Editor")
    static FString CompileAndSave(UBlueprint* Blueprint, bool bSave = true);
};
