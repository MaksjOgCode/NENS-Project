using UnrealBuildTool;
using System.Collections.Generic;
public class NENS_ProjectTarget : TargetRules
{
    public NENS_ProjectTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("NENS_Project");
    }
}
