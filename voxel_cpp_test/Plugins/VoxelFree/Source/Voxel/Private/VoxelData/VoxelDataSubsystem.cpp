// Copyright 2021 Phyronnaz

#include "VoxelData/VoxelDataSubsystem.h"
#include "VoxelData/VoxelData.h"
#include "VoxelGenerators/VoxelGeneratorCache.h"
#include "VoxelGenerators/VoxelGeneratorInstance.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"

DEFINE_VOXEL_SUBSYSTEM_PROXY(UVoxelDataSubsystemProxy);

void FVoxelDataSubsystem::Create()
{
	Super::Create();

	if (Settings.DataOverride)
	{
		Data = Settings.DataOverride;
	}
	else
	{
		FVoxelDataSettings DataSettings;
		DataSettings.Depth = FVoxelUtilities::ConvertDepth(Settings.RenderOctreeChunkSize, DATA_CHUNK_SIZE, Settings.RenderOctreeDepth);
		DataSettings.WorldBounds = Settings.GetWorldBounds();

		DataSettings.Generator = GetSubsystemChecked<FVoxelGeneratorCache>().MakeGeneratorInstance(Settings.Generator);

		DataSettings.bEnableMultiplayer = Settings.bEnableMultiplayer;
		DataSettings.bEnableUndoRedo = Settings.bEnableUndoRedo;

		check(!Data);
		Data = FVoxelData::Create(DataSettings, Settings.DataOctreeInitialSubdivisionDepth);
	}
}

void FVoxelDataSubsystem::PreDestructor()
{
	Super::PreDestructor();

	Data.Reset();
}