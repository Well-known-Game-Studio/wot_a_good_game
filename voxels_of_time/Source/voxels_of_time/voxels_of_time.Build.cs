// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class voxels_of_time : ModuleRules
{
	public voxels_of_time(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule" });
    }
}
