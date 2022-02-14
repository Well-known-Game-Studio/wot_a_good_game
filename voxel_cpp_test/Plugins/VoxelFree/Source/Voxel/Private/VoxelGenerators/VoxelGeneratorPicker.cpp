// Copyright 2021 Phyronnaz

#include "VoxelGenerators/VoxelGeneratorPicker.h"
#include "VoxelGenerators/VoxelEmptyGenerator.h"
#include "VoxelMessages.h"

#include "UObject/Package.h"

inline void CheckOutputs(const UVoxelGenerator& Generator, const FVoxelGeneratorInstance& Instance)
{
#if VOXEL_DEBUG
	VOXEL_FUNCTION_COUNTER();

	FVoxelGeneratorOutputs Outputs = Generator.GetGeneratorOutputs();
	const auto Order = [](FName A, FName B) { return A.FastLess(B); };

	{
		TArray<FName> FloatOutputs;
		Instance.GetOutputsPtrMap<v_flt>().GenerateKeyArray(FloatOutputs);
		FloatOutputs.Sort(Order);
		Outputs.FloatOutputs.Sort(Order);
		ensure(FloatOutputs == Outputs.FloatOutputs); // Will fail if a graph failed to compile
	}

	{
		TArray<FName> IntOutputs;
		Instance.GetOutputsPtrMap<int32>().GenerateKeyArray(IntOutputs);
		IntOutputs.Sort(Order);
		Outputs.IntOutputs.Sort(Order);
		ensure(IntOutputs == Outputs.IntOutputs); // Will fail if a graph failed to compile
	}

	{
		TArray<FName> ColorOutputs;
		Instance.GetOutputsPtrMap<FColor>().GenerateKeyArray(ColorOutputs);
		ColorOutputs.Sort(Order);
		Outputs.ColorOutputs.Sort(Order);
		ensure(ColorOutputs == Outputs.ColorOutputs); // Will fail if a graph failed to compile
	}
#endif
}

TVoxelSharedRef<FVoxelGeneratorInstance> FVoxelGeneratorPicker::GetInstance() const
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	auto* Generator = GetGenerator();
	if (Generator)
	{
		const auto Instance = Generator->GetInstance(Parameters);
		CheckOutputs(*Generator, *Instance);
		return Instance;
	}
	else
	{
		return MakeVoxelShared<FVoxelEmptyGeneratorInstance>();
	}
}

TVoxelSharedRef<FVoxelTransformableGeneratorInstance> FVoxelTransformableGeneratorPicker::GetInstance() const
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	
	auto* Generator = GetGenerator();
	if (Generator)
	{
		const auto Instance = Generator->GetTransformableInstance(Parameters);
		CheckOutputs(*Generator, *Instance);
		return Instance;
	}
	else
	{
		return MakeVoxelShared<FVoxelTransformableEmptyGeneratorInstance>();
	}
}