// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelIntBox.h"
#include "VoxelRender/VoxelProcMeshTangent.h"
#include "VoxelRender/VoxelMaterialIndices.h"

class FVoxelData;
class FVoxelRuntimeSettings;
class FDistanceFieldVolumeData;

DECLARE_VOXEL_MEMORY_STAT(TEXT("Voxel Chunk Mesh Memory"), STAT_VoxelChunkMeshMemory, STATGROUP_VoxelMemory, VOXEL_API);

struct VOXEL_API FVoxelChunkMeshBuffers
{
	TArray<uint32> Indices;
	TArray<FVector> Positions;

	// Will not be set if bRenderWorld is false
	TArray<FVector> Normals;
	TArray<FVoxelProcMeshTangent> Tangents;
	TArray<FColor> Colors;
	TArray<TArray<FVector2D>> TextureCoordinates;

	TArray<uint8> TextureData;
	TArray<FBox> CollisionCubes;

	FBox Bounds;
	FGuid Guid; // Use to avoid rebuilding collisions when the mesh didn't change

	FVoxelChunkMeshBuffers() = default;
	~FVoxelChunkMeshBuffers()
	{
		DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelChunkMeshMemory, LastAllocatedSize);
	}

	int32 GetNumVertices() const
	{
		return Positions.Num();
	}

	void BuildAdjacency(TArray<uint32>& OutAdjacencyIndices) const;
	void OptimizeIndices();
	void Shrink();
	void ComputeBounds();

private:
	int32 LastAllocatedSize = 0;

	void UpdateStats();
};

struct FVoxelChunkMesh
{
public:
	bool IsSingle() const
	{
		return bSingleBuffers;
	}
	bool IsEmpty() const
	{
		return bSingleBuffers ? SingleBuffers->Indices.Num() == 0 : Map.Num() == 0;
	}

	TVoxelSharedPtr<const FVoxelChunkMeshBuffers> GetSingleBuffers() const
	{
		ensure(IsSingle());
		ensure(SingleBuffers.IsValid());
		return SingleBuffers;
	}
	TVoxelSharedPtr<const FDistanceFieldVolumeData> GetDistanceFieldVolumeData() const
	{
		return DistanceFieldVolumeData;
	}
	TVoxelSharedPtr<const FVoxelChunkMeshBuffers> FindBuffer(const FVoxelMaterialIndices& MaterialIndices) const
	{
		ensure(!IsSingle());
		return Map.FindRef(MaterialIndices);
	}

public:
	void SetIsSingle(bool bIsSingle)
	{
		bSingleBuffers = bIsSingle;
	}
	FVoxelChunkMeshBuffers& CreateSingleBuffers()
	{
		ensure(IsSingle());
		ensure(!SingleBuffers.IsValid());
		SingleBuffers = MakeVoxelShared<FVoxelChunkMeshBuffers>();
		return *SingleBuffers;
	}
	FVoxelChunkMeshBuffers& FindOrAddBuffer(FVoxelMaterialIndices MaterialIndices, bool& bOutAdded)
	{
		ensure(!IsSingle());
		auto* BufferPtr = Map.Find(MaterialIndices);
		bOutAdded = BufferPtr == nullptr;
		if (!BufferPtr)
		{
			BufferPtr = &Map.Add(MaterialIndices, MakeVoxelShared<FVoxelChunkMeshBuffers>());
		}
		return **BufferPtr;
	}
	
public:
	void BuildDistanceField(int32 LOD, const FIntVector& Position, const FVoxelData& Data, const FVoxelRuntimeSettings& Settings);
	
public:
	template<typename T>
	void IterateBuffers(T Lambda)
	{
		if (bSingleBuffers)
		{
			Lambda(*SingleBuffers);
		}
		else
		{
			for (auto& It : Map)
			{
				Lambda(*It.Value);
			}
		}
	}
	template<typename T>
	void IterateBuffers(T Lambda) const
	{
		if (bSingleBuffers)
		{
			Lambda(static_cast<const FVoxelChunkMeshBuffers&>(*SingleBuffers));
		}
		else
		{
			for (auto& It : Map)
			{
				Lambda(static_cast<const FVoxelChunkMeshBuffers&>(*It.Value));
			}
		}
	}
	
	template<typename T>
	void IterateMaterials(T Lambda) const
	{
		ensure(!IsSingle());
		for (auto& It : Map)
		{
			Lambda(It.Key);
		}
	}

private:
	bool bSingleBuffers = false;
	TVoxelSharedPtr<FVoxelChunkMeshBuffers> SingleBuffers;
	TMap<FVoxelMaterialIndices, TVoxelSharedPtr<FVoxelChunkMeshBuffers>> Map;
	
	TVoxelSharedPtr<FDistanceFieldVolumeData> DistanceFieldVolumeData;
};