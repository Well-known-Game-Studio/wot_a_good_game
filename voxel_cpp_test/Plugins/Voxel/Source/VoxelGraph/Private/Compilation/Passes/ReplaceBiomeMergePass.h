// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelCompilationPass.h"

struct FVoxelReplaceBiomeMergePass
{
	VOXEL_PASS_BODY(FVoxelReplaceBiomeMergePass);

	static void Apply(FVoxelGraphCompiler& Compiler);
};

