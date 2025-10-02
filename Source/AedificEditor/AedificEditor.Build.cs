// Copyright (c) 2025 Ampere Games.

using UnrealBuildTool;

public class AedificEditor : ModuleRules
{
	public AedificEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Aedific",
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "PlacementMode",
                "Projects",
                "SlateCore",
            }
		);
    }
}
