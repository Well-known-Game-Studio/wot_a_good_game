// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class VoxelRPG : ModuleRules
{
	public VoxelRPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Niagara" });

		// Slate UI and Geometry Scripting modules
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"GeometryCore",
			"GeometryFramework",
			"GeometryScriptingCore",
			"DynamicMesh",
		});
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
