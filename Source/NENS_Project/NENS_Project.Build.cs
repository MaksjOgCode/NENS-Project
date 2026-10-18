using UnrealBuildTool;
public class NENS_Project : ModuleRules
{
    public NENS_Project(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore" });
        RuntimeDependencies.Add("$(TargetOutputDir)/ThirdPartyNotices/IMFellEnglish-OFL.txt",
            System.IO.Path.Combine(ModuleDirectory, "../../Content/NENS/UI/Fonts/Source/IMFellEnglish/OFL.txt"), StagedFileType.NonUFS);
    }
}
