using UnrealBuildTool;

public class CrimsonWarsNative : ModuleRules
{
    public CrimsonWarsNative(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "WebBrowserWidget",
            "WebSockets",
            "HTTP",
            "ImageWrapper",
            "Json",
            "JsonUtilities",
            "Niagara"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate",
            "SlateCore"
        });
    }
}
