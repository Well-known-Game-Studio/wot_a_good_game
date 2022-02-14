// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class voxel_cpp_test : ModuleRules
{
	public voxel_cpp_test(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule" });
    }
}
