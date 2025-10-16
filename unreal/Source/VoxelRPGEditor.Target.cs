// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class VoxelRPGEditorTarget : TargetRules
{
	public VoxelRPGEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		CppStandard = CppStandardVersion.Cpp20;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		MacPlatform.bUseDSYMFiles = false;
		IOSPlatform.bGeneratedSYM = true;

		ExtraModuleNames.AddRange( new string[] { "VoxelRPG" } );
	}
}
