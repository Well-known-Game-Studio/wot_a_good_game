// Copyright 2021 Phyronnaz

#include "VoxelDebug/VoxelDebugManager.h"
#include "VoxelDebug/VoxelDebugUtilities.h"
#include "VoxelData/VoxelData.h"
#include "VoxelRender/IVoxelLODManager.h"
#include "VoxelRender/IVoxelRenderer.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelComponents/VoxelInvokerComponent.h"
#include "VoxelTools/VoxelDataTools.h"
#include "VoxelTools/VoxelSurfaceTools.h"
#include "VoxelTools/VoxelBlueprintLibrary.h"
#include "VoxelMessages.h"
#include "VoxelWorld.h"
#include "VoxelPool.h"
#include "VoxelThreadPool.h"
#include "VoxelUtilities/VoxelConsoleUtilities.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<int32> CVarShowUpdatedChunks(
	TEXT("voxel.renderer.ShowUpdatedChunks"),
	0,
	TEXT("If true, will show the chunks recently updated"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowRenderChunks(
	TEXT("voxel.renderer.ShowRenderChunks"),
	0,
	TEXT("If true, will show the render chunks"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowMultiplayerSyncedChunks(
	TEXT("voxel.multiplayer.ShowSyncedChunks"),
	0,
	TEXT("If true, will show the synced chunks"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowValuesState(
	TEXT("voxel.data.ShowValuesState"),
	0,
	TEXT("If true, will show the values data chunks and their status (cached/created...)"),
	ECVF_Default);
static TAutoConsoleVariable<int32> CVarShowMaterialsState(
	TEXT("voxel.data.ShowMaterialsState"),
	0,
	TEXT("If true, will show the materials data chunks and their status (cached/created...)"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowDirtyValues(
	TEXT("voxel.data.ShowDirtyValues"),
	0,
	TEXT("If true, will show the data chunks with dirty values"),
	ECVF_Default);
static TAutoConsoleVariable<int32> CVarShowDirtyMaterials(
	TEXT("voxel.data.ShowDirtyMaterials"),
	0,
	TEXT("If true, will show the data chunks with dirty materials"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarFreezeDebug(
	TEXT("voxel.FreezeDebug"),
	0,
	TEXT("If true, won't clear previous frames boxes"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarDebugDrawTime(
	TEXT("voxel.debug.DrawTime"),
	1,
	TEXT("Draw time will be multiplied by this"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowChunksEmptyStates(
	TEXT("voxel.renderer.ShowChunksEmptyStates"),
	0,
	TEXT("If true, will show updated chunks empty state, only if non-empty. Use ShowAllChunksEmptyStates to show empty too."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowAllChunksEmptyStates(
	TEXT("voxel.renderer.ShowAllChunksEmptyStates"),
	0,
	TEXT("If true, will show updated chunks empty state, both empty and non-empty. Use ShowChunksEmptyStates to only show non-empty ones"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowWorldBounds(
	TEXT("voxel.ShowWorldBounds"),
	0,
	TEXT("If true, will show the world bounds"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowInvokers(
	TEXT("voxel.ShowInvokers"),
	0,
	TEXT("If true, will show the voxel invokers"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowDirtyVoxels(
	TEXT("voxel.data.ShowDirtyVoxels"),
	0,
	TEXT("If true, will show every dirty voxel in the scene"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarShowPlaceableItemsChunks(
	TEXT("voxel.data.ShowPlaceableItemsChunks"),
	0,
	TEXT("If true, will show every chunk that has a placeable item"),
	ECVF_Default);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static FAutoConsoleCommandWithWorld ClearChunksEmptyStatesCmd(
	TEXT("voxel.renderer.ClearChunksEmptyStates"),
	TEXT("Clear the empty states debug"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World) { World.GetSubsystemChecked<FVoxelDebugManager>().ClearChunksEmptyStates(); }));

static FAutoConsoleCommandWithWorld UpdateAllCmd(
	TEXT("voxel.renderer.UpdateAll"),
	TEXT("Update all the chunks in all the voxel world in the scene"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World) { UVoxelBlueprintLibrary::UpdateBounds(&World, FVoxelIntBox::Infinite); }));

static FAutoConsoleCommandWithWorld RecomputeComponentPositionsCmd(
	TEXT("voxel.RecomputeComponentPositions"),
	TEXT("Recompute the positions of all the components in all the voxel world in the scene"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World) { UVoxelBlueprintLibrary::RecomputeComponentPositions(&World); }));

static FAutoConsoleCommandWithWorld ForceLODsUpdateCmd(
	TEXT("voxel.renderer.ForceLODUpdate"),
	TEXT("Update the LODs"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World) { World.GetSubsystemChecked<IVoxelLODManager>().ForceLODsUpdate(); }));

static FAutoConsoleCommandWithWorld CacheAllValuesCmd(
	TEXT("voxel.data.CacheAllValues"),
	TEXT("Cache all values"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::CacheValues(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld CacheAllMaterialsCmd(
	TEXT("voxel.data.CacheAllMaterials"),
	TEXT("Cache all materials"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::CacheMaterials(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld ClearAllCachedValuesCmd(
	TEXT("voxel.data.ClearAllCachedValues"),
	TEXT("Clear all cached values"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::ClearCachedValues(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld ClearAllCachedMaterialsCmd(
	TEXT("voxel.data.ClearAllCachedMaterials"),
	TEXT("Clear all cached materials"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::ClearCachedMaterials(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld CheckForSingleValuesCmd(
	TEXT("voxel.data.CheckForSingleValues"),
	TEXT("Check if values in a chunk are all the same, and if so only store one"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::CheckForSingleValues(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld CheckForSingleMaterialsCmd(
	TEXT("voxel.data.CheckForSingleMaterials"),
	TEXT("Check if materials in a chunk are all the same, and if so only store one"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::CheckForSingleMaterials(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld RoundVoxelsCmd(
	TEXT("voxel.data.RoundVoxels"),
	TEXT("Round all voxels that do not impact the surface nor the normals"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::RoundVoxels(&World, FVoxelIntBox::Infinite);
			if (World.GetSubsystemChecked<FVoxelData>().bEnableUndoRedo) UVoxelBlueprintLibrary::SaveFrame(&World);
		}));

static FAutoConsoleCommandWithWorld ClearUnusedMaterialsCmd(
	TEXT("voxel.data.ClearUnusedMaterials"),
	TEXT("Will clear all materials that do not affect the surface to improve compression"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::ClearUnusedMaterials(&World, FVoxelIntBox::Infinite);
			if (World.GetSubsystemChecked<FVoxelData>().bEnableUndoRedo) UVoxelBlueprintLibrary::SaveFrame(&World);
		}));

static FAutoConsoleCommandWithWorld RegenerateAllSpawnersCmd(
	TEXT("voxel.foliage.RegenerateAll"),
	TEXT("Regenerate all foliage that can be regenerated"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelBlueprintLibrary::RegenerateSpawners(&World, FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld CompressIntoHeightmapCmd(
	TEXT("voxel.data.CompressIntoHeightmap"),
	TEXT("Update the heightmap to match the voxel world data"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::CompressIntoHeightmap(&World);
			UVoxelBlueprintLibrary::UpdateBounds(&World, FVoxelIntBox::Infinite);
			if (World.GetSubsystemChecked<FVoxelData>().bEnableUndoRedo) UVoxelBlueprintLibrary::SaveFrame(&World);
		}));

static FAutoConsoleCommandWithWorld RoundToGeneratorCmd(
	TEXT("voxel.data.RoundToGenerator"),
	TEXT("Set the voxels back to the generator value if all the voxels in a radius of 2 have the same sign as the generator"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelDataTools::RoundToGenerator(&World, FVoxelIntBox::Infinite);
			UVoxelBlueprintLibrary::UpdateBounds(&World, FVoxelIntBox::Infinite);
			if (World.GetSubsystemChecked<FVoxelData>().bEnableUndoRedo) UVoxelBlueprintLibrary::SaveFrame(&World);
		}));

static FAutoConsoleCommandWithWorld CompactTexturePoolCmd(
	TEXT("voxel.texturepool.compact"),
	TEXT("Reallocate all the entries, reducing fragmentation & saving memory"),
	FVoxelUtilities::CreateVoxelWorldCommand([](AVoxelWorld& World)
		{
			UVoxelBlueprintLibrary::CompactVoxelTexturePool(&World);
		}));

static bool GShowCollisionAndNavmeshDebug = false;

static FAutoConsoleCommandWithWorldAndArgs ShowCollisionAndNavmeshDebugCmd(
	TEXT("voxel.renderer.ShowCollisionAndNavmeshDebug"),
	TEXT("If true, will show chunks used for collisions/navmesh and will color all chunks according to their usage"),
	FVoxelUtilities::CreateVoxelWorldCommandWithArgs([](AVoxelWorld& World, const TArray<FString>& Args)
		{
			if (Args.Num() == 0)
			{
				GShowCollisionAndNavmeshDebug = !GShowCollisionAndNavmeshDebug;
			}
			else if (Args[0] == "0")
			{
				GShowCollisionAndNavmeshDebug = false;
			}
			else
			{
				GShowCollisionAndNavmeshDebug = true;
			}

			World.GetSubsystemChecked<IVoxelLODManager>().UpdateBounds(FVoxelIntBox::Infinite);
		}));

static FAutoConsoleCommandWithWorld RebaseOntoCameraCmd(
	TEXT("voxel.RebaseOntoCamera"),
	TEXT("Call SetWorldOriginLocation so that the camera is at 0 0 0"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		auto* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
		if (ensure(CameraManager))
		{
			const FVector Position = CameraManager->GetCameraLocation();
			UGameplayStatics::SetWorldOriginLocation(World, UGameplayStatics::GetWorldOriginLocation(World) + FIntVector(Position));
		}
	}));

static FAutoConsoleCommand CmdLogMemoryStats(
    TEXT("voxel.LogMemoryStats"),
    TEXT(""),
    FConsoleCommandDelegate::CreateStatic(&UVoxelBlueprintLibrary::LogMemoryStats));

static void LogSecondsPerCycles()
{
    LOG_VOXEL(Log, TEXT("SECONDS PER CYCLES: %e"), FPlatformTime::GetSecondsPerCycle());
}

static FAutoConsoleCommand CmdLogSecondsPerCycles(
    TEXT("voxel.debug.LogSecondsPerCycles"),
    TEXT(""),
    FConsoleCommandDelegate::CreateStatic(&LogSecondsPerCycles));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelGlobalDebugManager::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();
	
	const auto Counters = GVoxelThreadPool->GetGlobalCounters();
	
	const int32 PoolTaskCount = Counters.GetTotalNumTasks();

	const int32 MesherTaskCount =
		Counters.GetNumTasksForType(EVoxelTaskType::ChunksMeshing) +
		Counters.GetNumTasksForType(EVoxelTaskType::VisibleChunksMeshing) +
		Counters.GetNumTasksForType(EVoxelTaskType::CollisionsChunksMeshing) +
		Counters.GetNumTasksForType(EVoxelTaskType::VisibleCollisionsChunksMeshing) +
		Counters.GetNumTasksForType(EVoxelTaskType::MeshMerge);

	const int32 FoliageTaskCount =
		Counters.GetNumTasksForType(EVoxelTaskType::FoliageBuild) +
		Counters.GetNumTasksForType(EVoxelTaskType::HISMBuild);

	const int32 EditTaskCount = Counters.GetNumTasksForType(EVoxelTaskType::AsyncEditFunctions);
	const int32 LODTaskCount = Counters.GetNumTasksForType(EVoxelTaskType::RenderOctree);
	const int32 CollisionTaskCount = Counters.GetNumTasksForType(EVoxelTaskType::CollisionCooking);
	
	if (PoolTaskCount > 0)
	{
		const FString Message = FString::Printf(TEXT("Voxel tasks: %d (mesher: %d; foliage: %d; edits: %d; LOD: %d; Collision: %d) %d threads"),
			PoolTaskCount,
			MesherTaskCount,
			FoliageTaskCount,
			EditTaskCount,
			LODTaskCount,
			CollisionTaskCount,
			CVarVoxelThreadingNumThreads.GetValueOnGameThread());
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DeltaTime * 1.5f, FColor::White, Message);
	}

	return true;
}

FVoxelGlobalDebugManager* GVoxelDebugManager = nullptr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline float GetBoundsThickness(const FVoxelIntBox& Bounds)
{
	return Bounds.Size().GetMax();
}

#define DRAW_BOUNDS(Bounds, Color, bThick) UVoxelDebugUtilities::DrawDebugIntBox(World, Bounds, DebugDT, bThick ? GetBoundsThickness(Bounds) : 0, FLinearColor(Color));
#define DRAW_BOUNDS_ARRAY(BoundsArray, Color, bThick) for (auto& Bounds : BoundsArray) { DRAW_BOUNDS(Bounds, FColorList::Color, bThick) }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_SUBSYSTEM_PROXY(UVoxelDebugSubsystemProxy);

void FVoxelDebugManager::Destroy()
{
	Super::Destroy();

	StopTicking();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelDebugManager::ReportUpdatedChunks(TFunction<TArray<FVoxelIntBox>()> InUpdatedChunks)
{
	if (CVarShowUpdatedChunks.GetValueOnGameThread())
	{
		VOXEL_ASYNC_FUNCTION_COUNTER();
		UpdatedChunks = InUpdatedChunks();
	
		FString Log = "Updated chunks: ";
		for (auto& Bounds : UpdatedChunks)
		{
			Log += Bounds.ToString() + "; ";
		}
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), 1, FColor::Blue, Log);
		LOG_VOXEL(Log, TEXT("%s"), *Log);
	}
}

void FVoxelDebugManager::ReportRenderChunks(TFunction<TArray<FVoxelIntBox>()> InRenderChunks)
{
	if (CVarShowRenderChunks.GetValueOnGameThread())
	{
		VOXEL_ASYNC_FUNCTION_COUNTER();
		RenderChunks = InRenderChunks();
	}
}

void FVoxelDebugManager::ReportMultiplayerSyncedChunks(TFunction<TArray<FVoxelIntBox>()> InMultiplayerSyncedChunks)
{
	if (CVarShowMultiplayerSyncedChunks.GetValueOnGameThread())
	{
		VOXEL_ASYNC_FUNCTION_COUNTER();
		MultiplayerSyncedChunks = InMultiplayerSyncedChunks();
	}
}

void FVoxelDebugManager::ReportMeshTasksCallbacksQueueNum(int32 Num)
{
	MeshTasksCallbacksQueueNum = Num;
}

void FVoxelDebugManager::ReportMeshActionQueueNum(int32 Num)
{
	MeshActionQueueNum = Num;
}

void FVoxelDebugManager::ReportChunkEmptyState(const FVoxelIntBox& Bounds, bool bIsEmpty)
{
	ChunksEmptyStates.Emplace(FChunkEmptyState{ Bounds, bIsEmpty });
}

void FVoxelDebugManager::ClearChunksEmptyStates()
{
	ChunksEmptyStates.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelDebugManager::ShowCollisionAndNavmeshDebug()
{
	return GShowCollisionAndNavmeshDebug;
}

FColor FVoxelDebugManager::GetCollisionAndNavmeshDebugColor(bool bEnableCollisions, bool bEnableNavmesh)
{
	if (bEnableCollisions && bEnableNavmesh)
	{
		return FColor::Yellow;
	}
	if (bEnableCollisions)
	{
		return FColor::Blue;
	}
	if (bEnableNavmesh)
	{
		return FColor::Green;
	}
	return FColor::White;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelDebugManager::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	const AVoxelWorld* World = Settings.VoxelWorld.Get();
	if (!World || Settings.bDisableDebugManager) 
	{
		return;
	}

	const float DebugDT = DeltaTime * 1.5f * CVarDebugDrawTime.GetValueOnGameThread();

	if (CVarShowRenderChunks.GetValueOnGameThread())
	{
		DRAW_BOUNDS_ARRAY(RenderChunks, Grey, false);
	}
	if (CVarShowUpdatedChunks.GetValueOnGameThread())
	{
		DRAW_BOUNDS_ARRAY(UpdatedChunks, Blue, true);
	}
	if (CVarShowMultiplayerSyncedChunks.GetValueOnGameThread())
	{
		DRAW_BOUNDS_ARRAY(MultiplayerSyncedChunks, Blue, true);
	}
	if (CVarShowWorldBounds.GetValueOnGameThread())
	{
		DRAW_BOUNDS(Settings.GetWorldBounds(), FColorList::Red, true);
	}
	if (!Settings.bDisableOnScreenMessages)
	{
		if (MeshTasksCallbacksQueueNum > 0)
		{
			GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, FColor::White, FString::Printf(TEXT("Mesh tasks callbacks queued: %d"), MeshTasksCallbacksQueueNum));
		}
		if (MeshActionQueueNum > 0)
		{
			GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, FColor::White, FString::Printf(TEXT("Mesh actions queued: %d"), MeshActionQueueNum));
		}
	}
	if (!CVarFreezeDebug.GetValueOnGameThread())
	{
		UpdatedChunks.Reset();
		MultiplayerSyncedChunks.Reset();
	}

	if (CVarShowInvokers.GetValueOnGameThread())
	{
		const FColor LocalInvokerColor = FColor::Green;
		const FColor RemoteInvokerColor = FColor::Silver;
		const FColor LODColor = FColor::Red;
		const FColor CollisionsColor = FColor::Blue;
		const FColor NavmeshColor = FColor::Green;
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, LocalInvokerColor, TEXT("Local Invokers"));
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, RemoteInvokerColor, TEXT("Remote Invokers"));
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, LODColor, TEXT("Invokers LOD Bounds"));
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, CollisionsColor, TEXT("Invokers Collisions Bounds"));
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, NavmeshColor, TEXT("Invokers Navmesh Bounds"));
		
		for (auto& Invoker : UVoxelInvokerComponentBase::GetInvokers(World))
		{
			if (Invoker.IsValid())
			{
				DrawDebugPoint(
					World->GetWorld(),
					World->LocalToGlobal(Invoker->GetInvokerVoxelPosition(World)),
					100,
					Invoker->IsLocalInvoker() ? LocalInvokerColor : RemoteInvokerColor,
					false,
					DebugDT);

				const auto InvokerSettings = Invoker->GetInvokerSettings(World);

				if (InvokerSettings.bUseForLOD)
				{
					DRAW_BOUNDS(InvokerSettings.LODBounds, LODColor, true);
				}
				if (InvokerSettings.bUseForCollisions)
				{
					DRAW_BOUNDS(InvokerSettings.CollisionsBounds, CollisionsColor, true);
				}
				if (InvokerSettings.bUseForNavmesh)
				{
					DRAW_BOUNDS(InvokerSettings.NavmeshBounds, NavmeshColor, true);
				}
			}
		}
	}

	if (CVarShowChunksEmptyStates.GetValueOnGameThread())
	{
		const static FColor NotEmpty = FColorList::Brown;

		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, NotEmpty, TEXT("Not empty chunks (range analysis failed)"));

		for (auto& EmptyState : ChunksEmptyStates)
		{
			if (!EmptyState.bIsEmpty)
			{
				DRAW_BOUNDS(EmptyState.Bounds, NotEmpty, false);
			}
		}
	}
	if (CVarShowAllChunksEmptyStates.GetValueOnGameThread())
	{
		const static FColor Empty = FColorList::Green;
		const static FColor NotEmpty = FColorList::Brown;

		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, Empty, TEXT("Empty chunks (range analysis successful)"));
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, NotEmpty, TEXT("Not empty chunks (range analysis failed)"));

		for (auto& EmptyState : ChunksEmptyStates)
		{
			if (EmptyState.bIsEmpty)
			{
				DRAW_BOUNDS(EmptyState.Bounds, Empty, false);
			}
			else
			{
				DRAW_BOUNDS(EmptyState.Bounds, NotEmpty, false);
			}
		}
	}

	const FColor SingleColor = FColorList::Green;
	const FColor SingleDirtyColor = FColorList::Blue;
	const FColor CachedColor = FColorList::Yellow;
	const FColor DirtyColor = FColorList::Red;
	
	const UVoxelDebugUtilities::FDrawDataOctreeSettings DrawDataOctreeSettings
	{
		World,
		DebugDT,
		false,
		false,
		SingleColor,
		SingleDirtyColor,
		CachedColor,
		DirtyColor
	};
	
	FVoxelData& Data = GetSubsystemChecked<FVoxelData>();
	
	if (CVarShowValuesState.GetValueOnGameThread())
	{
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, FColor::White, "Values state:");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, DirtyColor, "Dirty");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, CachedColor, "Cached");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, SingleColor, "Single Item Stored");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, SingleDirtyColor, "Single Item Stored - Dirty");

		auto LocalDrawDataOctreeSettings = DrawDataOctreeSettings;
		LocalDrawDataOctreeSettings.bShowSingle = true;
		LocalDrawDataOctreeSettings.bShowCached = true;

		FVoxelReadScopeLock Lock(Data, FVoxelIntBox::Infinite, FUNCTION_FNAME);
		UVoxelDebugUtilities::DrawDataOctreeImpl<FVoxelValue>(Data, LocalDrawDataOctreeSettings);
	}
	if (CVarShowMaterialsState.GetValueOnGameThread())
	{
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, FColor::White, "Materials state:");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, DirtyColor, "Dirty");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, CachedColor, "Cached");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, SingleColor, "Single Item Stored");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, SingleDirtyColor, "Single Item Stored - Dirty");

		auto LocalDrawDataOctreeSettings = DrawDataOctreeSettings;
		LocalDrawDataOctreeSettings.bShowSingle = true;
		LocalDrawDataOctreeSettings.bShowCached = true;
		
		FVoxelReadScopeLock Lock(Data, FVoxelIntBox::Infinite, FUNCTION_FNAME);
		UVoxelDebugUtilities::DrawDataOctreeImpl<FVoxelMaterial>(Data, LocalDrawDataOctreeSettings);
	}
	
	if (CVarShowDirtyValues.GetValueOnGameThread())
	{
		auto LocalDrawDataOctreeSettings = DrawDataOctreeSettings;
		LocalDrawDataOctreeSettings.bShowSingle = false;
		LocalDrawDataOctreeSettings.bShowCached = false;
		
		FVoxelReadScopeLock Lock(Data, FVoxelIntBox::Infinite, FUNCTION_FNAME);
		UVoxelDebugUtilities::DrawDataOctreeImpl<FVoxelValue>(Data, LocalDrawDataOctreeSettings);
	}
	if (CVarShowDirtyMaterials.GetValueOnGameThread())
	{
		auto LocalDrawDataOctreeSettings = DrawDataOctreeSettings;
		LocalDrawDataOctreeSettings.bShowSingle = false;
		LocalDrawDataOctreeSettings.bShowCached = false;
		
		FVoxelReadScopeLock Lock(Data, FVoxelIntBox::Infinite, FUNCTION_FNAME);
		UVoxelDebugUtilities::DrawDataOctreeImpl<FVoxelMaterial>(Data, LocalDrawDataOctreeSettings);
	}

	if (CVarShowPlaceableItemsChunks.GetValueOnGameThread())
	{
		FVoxelReadScopeLock Lock(Data, FVoxelIntBox::Infinite, FUNCTION_FNAME);
		FVoxelOctreeUtilities::IterateEntireTree(Data.GetOctree(), [&](const FVoxelDataOctreeBase& Octree)
		{
			if (Octree.IsLeafOrHasNoChildren() && Octree.GetItemHolder().NumItems() > 0)
			{
				ensureThreadSafe(Octree.IsLockedForRead());
				UVoxelDebugUtilities::DrawDebugIntBox(World, Octree.GetBounds(), DebugDT, 0, FColorList::Red);
			}
		});
	}
	
	if (GShowCollisionAndNavmeshDebug)
	{
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, GetCollisionAndNavmeshDebugColor(true, false), "Chunks with collisions");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, GetCollisionAndNavmeshDebugColor(false, true), "Chunks with navmesh");
		GEngine->AddOnScreenDebugMessage(OBJECT_LINE_ID(), DebugDT, GetCollisionAndNavmeshDebugColor(true, true), "Chunks with navmesh and collision");
	}
	if (CVarShowDirtyVoxels.GetValueOnGameThread())
	{
		FVoxelDataUtilities::IterateDirtyDataInBounds<FVoxelValue>(
			Data,
			FVoxelIntBox::Infinite,
			[&](int32 X, int32 Y, int32 Z, const FVoxelValue& Value)
			{
				DrawDebugPoint(
					World->GetWorld(),
					World->LocalToGlobal(FIntVector(X, Y, Z)),
					2,
					Value.IsEmpty() ? FColor::Blue : FColor::Red,
					false,
					DebugDT);
			});
	}
}