using UnrealBuildTool;
using System.Collections.Generic;

public class CrimsonWarsNativeEditorTarget : TargetRules
{
    public CrimsonWarsNativeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CrimsonWarsNative");
    }
}
