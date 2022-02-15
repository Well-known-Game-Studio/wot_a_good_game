// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelCompilationPass.h"

struct FVoxelReplaceLocalVariablesPass
{
	VOXEL_PASS_BODY(FVoxelReplaceLocalVariablesPass);
	
	static void Apply(FVoxelGraphCompiler& Compiler);
};
