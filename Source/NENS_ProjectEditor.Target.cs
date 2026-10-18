using UnrealBuildTool;
public class NENS_ProjectEditorTarget : TargetRules
{
    public NENS_ProjectEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "NENS_Project", "NENSInspectionEditor" });
    }
}
