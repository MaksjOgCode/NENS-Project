using UnrealBuildTool;

public class NENSInspectionEditor : ModuleRules
{
    public NENSInspectionEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "NENS_Project", "UnrealEd", "Kismet", "KismetCompiler", "BlueprintGraph",
            "UMGEditor", "UMG", "Json", "JsonUtilities", "BlueprintEditorLibrary",
            "Slate", "SlateCore", "InputCore"
        });
    }
}
