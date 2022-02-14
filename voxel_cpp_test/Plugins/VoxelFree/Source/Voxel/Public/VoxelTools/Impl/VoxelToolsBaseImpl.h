// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelEnums.h"
#include "VoxelValue.h"
#include "VoxelVector.h"
#include "VoxelIntBox.h"
#include "VoxelData/VoxelDataImpl.h"

class AVoxelWorld;
class FVoxelData;

struct FVoxelIntBox;
struct FModifiedVoxelValue;
struct FVoxelPaintMaterial;

class VOXEL_API FVoxelToolsBaseImpl
{
public:
	template<typename TData>
	static bool IsDataMultiThreaded(const TData& Data);
	
	template<typename TData>
	static FVoxelData& GetActualData(TData& Data);
	
	template<typename TData>
	static auto* GetModifiedValues(TData& Data);
	
	template<typename T>
	static auto* GetModifiedValues(TVoxelDataImpl<T>& Data);
};