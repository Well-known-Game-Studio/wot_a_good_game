// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelCompilationPass.h"

struct FVoxelCompactPassthroughsPass
{
	VOXEL_PASS_BODY(FVoxelCompactPassthroughsPass);

	static void Apply(FVoxelGraphCompiler& Compiler);
};
