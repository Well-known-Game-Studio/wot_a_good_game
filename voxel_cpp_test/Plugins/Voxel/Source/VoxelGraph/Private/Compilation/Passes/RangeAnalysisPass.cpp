// Copyright 2020 Phyronnaz

#include "Compilation/Passes/RangeAnalysisPass.h"
#include "Compilation/VoxelGraphCompiler.h"
#include "Compilation/VoxelGraphCompilerHelpers.h"
#include "Compilation/VoxelDefaultCompilationNodes.h"

void FVoxelDisconnectRangeAnalysisConstantsPass::Apply(FVoxelGraphCompiler& Compiler)
{
	for (auto* Node : Compiler.GetAllNodesCopy())
	{
		if (Node->IsA<FVoxelRangeAnalysisConstantCompilationNode>())
		{
			FVoxelGraphCompilerHelpers::BreakNodeLinks<EVoxelPinIter::Input>(*Node);
		}
	}
}

void FVoxelRemoveAllSeedNodesPass::Apply(FVoxelGraphCompiler& Compiler)
{
	for (auto* Node : Compiler.GetAllNodesCopy())
	{
		if (Node->IsSeedNode())
		{
			Node->BreakAllLinks();
			Compiler.RemoveNode(Node);
		}
	}
}
