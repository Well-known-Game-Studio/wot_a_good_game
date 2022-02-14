// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelAsyncWork.h"
#include "VoxelPriorityHandler.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/WeakObjectPtrTemplates.h"

struct FVoxelProcMeshBuffers;
struct FVoxelSimpleCollisionData;
struct FVoxelProceduralMeshComponentMemoryUsage;
class UBodySetup;
class UVoxelProceduralMeshComponent;
class IVoxelProceduralMeshComponent_PhysicsCallbackHandler;

class IVoxelAsyncPhysicsCooker : public FVoxelAsyncWork
{
public:
	const uint64 UniqueId;
	const TWeakObjectPtr<UVoxelProceduralMeshComponent> Component;
	const TVoxelWeakPtr<IVoxelProceduralMeshComponent_PhysicsCallbackHandler> PhysicsCallbackHandler;
	
	const int32 LOD;
	const ECollisionTraceFlag CollisionTraceFlag;
	const bool bCleanCollisionMesh;
	const bool bSimpleCubicCollision;
	const int32 NumConvexHullsPerAxis;
	const TArray<TVoxelSharedPtr<const FVoxelProcMeshBuffers>> Buffers;
	const FTransform LocalToRoot;

	explicit IVoxelAsyncPhysicsCooker(UVoxelProceduralMeshComponent* Component);

	static IVoxelAsyncPhysicsCooker* CreateCooker(UVoxelProceduralMeshComponent* Component);
	
public:
	//~ Begin IVoxelAsyncPhysicsCooker Interface
	virtual bool Finalize(
		UBodySetup& BodySetup,
		TVoxelSharedPtr<FVoxelSimpleCollisionData>& OutSimpleCollisionData,
		FVoxelProceduralMeshComponentMemoryUsage& OutMemoryUsage) = 0;
protected:
	virtual void CookMesh() = 0;
	//~ End IVoxelAsyncPhysicsCooker Interface

protected:
	//~ Begin FVoxelAsyncWork Interface
	virtual void DoWork() override;
	virtual void PostDoWork() override;
	//~ End FVoxelAsyncWork Interface
};