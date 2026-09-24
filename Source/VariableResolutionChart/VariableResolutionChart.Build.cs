// VariableResolutionChart.Build.cs
using UnrealBuildTool;

public class VariableResolutionChart : ModuleRules
{
    public VariableResolutionChart(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}