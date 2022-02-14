// Copyright 2021 Phyronnaz

#include "VoxelTools/Gen/VoxelToolsBase.h"
#include "VoxelWorld.h"

template<typename T>
FVoxelIntBox GetModifiedVoxelsBounds(const TArray<T>& ModifiedVoxels)
{
	FVoxelIntBoxWithValidity Bounds;
	for (auto& ModifiedVoxel : ModifiedVoxels)
	{
		Bounds += ModifiedVoxel.Position;
	}
	return Bounds.IsValid() ? Bounds.GetBox() : FVoxelIntBox();
}

FVoxelIntBox UVoxelToolsBase::GetModifiedVoxelValuesBounds(const TArray<FModifiedVoxelValue>& ModifiedVoxels)
{
	VOXEL_FUNCTION_COUNTER();
	return GetModifiedVoxelsBounds(ModifiedVoxels);
}

FVoxelIntBox UVoxelToolsBase::GetModifiedVoxelMaterialsBounds(const TArray<FModifiedVoxelMaterial>& ModifiedVoxels)
{
	VOXEL_FUNCTION_COUNTER();
	return GetModifiedVoxelsBounds(ModifiedVoxels);
}

bool UVoxelToolsBase::IsSingleIndexWorld(AVoxelWorld* VoxelWorld)
{
	return ensure(VoxelWorld) && VoxelWorld->MaterialConfig == EVoxelMaterialConfig::SingleIndex;
}