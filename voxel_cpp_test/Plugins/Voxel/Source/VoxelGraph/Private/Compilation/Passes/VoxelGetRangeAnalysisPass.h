// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelCompilationPass.h"

struct FVoxelGetRangeAnalysisPass
{
	VOXEL_PASS_BODY(FVoxelGetRangeAnalysisPass);
	
	static void Apply(FVoxelGraphCompiler& Compiler);
};
