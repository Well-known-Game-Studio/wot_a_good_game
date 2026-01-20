// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class VoxelRPGTarget : TargetRules
{
	public VoxelRPGTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		CppStandard = CppStandardVersion.Cpp20;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		MacPlatform.bUseDSYMFiles = false;
		IOSPlatform.bGeneratedSYM = true;

		ExtraModuleNames.AddRange( new string[] { "VoxelRPG" } );
	}
}
