// Copyright 2021 Phyronnaz

#include "VoxelRuntime.h"
#include "VoxelRuntimeActor.h"
#include "VoxelSubsystem.h"
#include "VoxelWorld.h"
#include "VoxelMessages.h"
#include "VoxelPriorityHandler.h"
#include "VoxelFoliageInterface.h"
#include "VoxelWorldRootComponent.h"
#include "VoxelPlaceableItems/VoxelPlaceableItemManager.h"
#include "VoxelRender/VoxelProceduralMeshComponent.h"
#include "VoxelRender/Renderers/VoxelDefaultRenderer.h"
#include "VoxelRender/LODManager/VoxelDefaultLODManager.h"
#include "VoxelRender/MaterialCollections/VoxelMaterialCollectionBase.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"
#include "UObject/UObjectHash.h"

FVoxelRuntimeSettings::FVoxelRuntimeSettings()
{
	PlayType = EVoxelPlayType::Game;
	SetFromRuntime(*GetDefault<AVoxelRuntimeActor>());
}

void FVoxelRuntimeSettings::SetFromRuntime(const AVoxelRuntimeActor& InRuntime)
{
#define SET(Name) Name = decltype(Name)(InRuntime.Name)

	Owner = const_cast<AVoxelRuntimeActor*>(&InRuntime);
	World = InRuntime.GetWorld();
	ComponentsOwner = const_cast<AVoxelRuntimeActor*>(&InRuntime);
	AttachRootComponent = InRuntime.GetRootComponent();
	VoxelRootComponent = Cast<UVoxelWorldRootComponent>(InRuntime.GetRootComponent());
	ensure(InRuntime.HasAnyFlags(RF_ClassDefaultObject) || (AttachRootComponent.IsValid() && VoxelRootComponent.IsValid()));
	
	Runtime = &InRuntime;
	VoxelWorld = Cast<const AVoxelWorld>(&InRuntime);

	SET(RendererSubsystem);
	SET(LODSubsystem);

	SET(VoxelSize);
	SET(Generator);
	SET(PlaceableItemManager);
	SET(bCreateWorldAutomatically);
	SET(bUseCameraIfNoInvokersFound);
	SET(bEnableUndoRedo);
	SET(bUseAbsoluteTransforms);
	SET(bMergeAssetActors);
	SET(bMergeDisableEditsBoxes);
	SET(bDisableOnScreenMessages);
	SET(bDisableDebugManager);

	SET(RenderOctreeDepth);
	SET(RenderOctreeChunkSize);
	SET(bUseCustomWorldBounds);
	SET(CustomWorldBounds);

	SET(MaxLOD);
	SET(MinLOD);
	SET(InvokerDistanceThreshold);
	SET(MinDelayBetweenLODUpdates);
	SET(bConstantLOD);

	SET(MaterialConfig);
	SET(bUseMaterialCollection);
	SET(VoxelMaterial);
	SET(MaterialCollection);
	SET(LODMaterials);
	SET(LODMaterialCollections);
	SET(UVConfig);
	SET(UVScale);
	SET(NormalConfig);
	SET(RGBHardness);
	SET(MaterialsHardness);
	SET(bHardColorTransitions);
	SET(bSplitSingleIndexTriangles);
	SET(HolesMaterials);
	SET(MaterialsMeshConfigs);
	SET(bHalfPrecisionCoordinates);
	SET(bInterpolateColors);
	SET(bInterpolateUVs);
	SET(bSRGBColors);

	SET(RenderType);
	SET(RenderSharpness);
	SET(bCreateMaterialInstances);
	SET(bDitherChunks);
	SET(ChunksDitheringDuration);
	SET(bCastFarShadow);
	SET(ProcMeshClass);
	SET(ChunksCullingLOD);
	SET(bRenderWorld);
	SET(bContributesToStaticLighting);
	SET(bUseStaticPath);
	SET(bStaticWorld);
	SET(bGreedyCubicMesher);
	SET(bSingleIndexGreedy);
	SET(TexturePoolTextureSize);
	SET(bOptimizeIndices);
	SET(bGenerateDistanceFields);
	SET(MaxDistanceFieldLOD);
	SET(DistanceFieldBoundsExtension);
	SET(DistanceFieldResolutionDivisor);
	SET(DistanceFieldSelfShadowBias);
	SET(bEnableTransitions);
	SET(bMergeChunks);
	SET(MergedChunksClusterSize);
	SET(bDoNotMergeCollisionsAndNavmesh);
	SET(BoundsExtension);

	SET(FoliageCollections);
	SET(FoliageWorldType);
	SET(bIsFourWayBlend);
	SET(HISMChunkSize);
	SET(FoliageCollisionDistanceInVoxel);
	SET(MaxNumberOfFoliageInstances);

	SET(bEnableCollisions);
	SET(CollisionPresets);
	SET(CollisionTraceFlag);
	SET(bComputeVisibleChunksCollisions);
	SET(VisibleChunksCollisionsMaxLOD);
	SET(bSimpleCubicCollision);
	SET(SimpleCubicCollisionLODBias);
	SET(NumConvexHullsPerAxis);
	SET(bCleanCollisionMeshes);

	SET(bEnableNavmesh);
	SET(bComputeVisibleChunksNavmesh);
	SET(VisibleChunksNavmeshMaxLOD);

	SET(PriorityCategories);
	SET(PriorityOffsets);
	SET(MeshUpdatesBudget);
	SET(EventsTickRate);
	SET(DataOctreeInitialSubdivisionDepth);

	SET(bEnableMultiplayer);
	SET(MultiplayerInterface);
	SET(MultiplayerSyncRate);

#undef SET

	Fixup();
}

void FVoxelRuntimeSettings::ConfigurePreview()
{
	PlayType = EVoxelPlayType::Preview;
	
	bEnableMultiplayer = false;
	bEnableUndoRedo = true;
	ProcMeshClass = nullptr;
	bCreateMaterialInstances = false;
	MeshUpdatesBudget = 1000;
	bStaticWorld = false;
	bConstantLOD = false;

	if (VoxelWorld.IsValid() && !VoxelWorld->bEnableFoliageInEditor)
	{
		FoliageCollections.Reset();
	}

	Fixup();
}

void FVoxelRuntimeSettings::Fixup()
{
	// Ordering matters! If you set something after it's checked, the logic will fail
	
	if (!ProcMeshClass)
	{
		ProcMeshClass = UVoxelProceduralMeshComponent::StaticClass();
	}
	if (RenderType == EVoxelRenderType::Cubic)
	{
		bHardColorTransitions = false;
	}
	if (bMergeChunks)
	{
		bCreateMaterialInstances = false;
	}
	else
	{
		bDoNotMergeCollisionsAndNavmesh = false;
	}
	if (!bCreateMaterialInstances)
	{
		bDitherChunks = false;
	}
	if (!bGenerateDistanceFields)
	{
		MaxDistanceFieldLOD = -1;
	}
	if (MaterialConfig == EVoxelMaterialConfig::RGB)
	{
		bUseMaterialCollection = false;
	}
	if (MaterialConfig == EVoxelMaterialConfig::MultiIndex)
	{
		bUseMaterialCollection = true;
	}
	if (bUseMaterialCollection)
	{
		bGreedyCubicMesher = false;
	}
	if (!bGreedyCubicMesher || 
		CollisionTraceFlag == CTF_UseComplexAsSimple) // Not needed when using only complex
	{
		bSimpleCubicCollision = false;
	}

	FVoxelUtilities::FixupChunkSize(RenderOctreeChunkSize, MESHER_CHUNK_SIZE);
	RenderOctreeDepth = FMath::Max(1, FVoxelUtilities::ClampDepth(RenderOctreeChunkSize, RenderOctreeDepth));
	
	MeshUpdatesBudget = FMath::Max(0.001f, MeshUpdatesBudget);
	RenderSharpness = FMath::Max(0, RenderSharpness);
	MergedChunksClusterSize = FMath::RoundUpToPowerOfTwo(FMath::Max(MergedChunksClusterSize, 2));
	SimpleCubicCollisionLODBias = FMath::Clamp(SimpleCubicCollisionLODBias, 0, 4);
	TexturePoolTextureSize = FMath::Clamp(TexturePoolTextureSize, 128, 16384);
	EventsTickRate = FMath::Max(SMALL_NUMBER, EventsTickRate);
	MultiplayerSyncRate = FMath::Max(SMALL_NUMBER, MultiplayerSyncRate);
	FoliageCollisionDistanceInVoxel = FMath::Max(0, FoliageCollisionDistanceInVoxel);
	HISMChunkSize = FMath::Max(32, HISMChunkSize);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelIntBox FVoxelRuntimeSettings::GetWorldBounds(bool bUseCustomWorldBounds, const FVoxelIntBox& CustomWorldBounds, int32 RenderOctreeChunkSize, int32 RenderOctreeDepth)
{
	if (bUseCustomWorldBounds)
	{
		return
			FVoxelUtilities::GetCustomBoundsForDepth(
				RenderOctreeChunkSize,
				FVoxelIntBox::SafeConstruct(CustomWorldBounds.Min, CustomWorldBounds.Max),
				RenderOctreeDepth);
	}
	else
	{
		return FVoxelUtilities::GetBoundsFromDepth(RenderOctreeChunkSize, RenderOctreeDepth);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelIntBox FVoxelRuntimeSettings::GetWorldBounds() const
{
	return GetWorldBounds(bUseCustomWorldBounds, CustomWorldBounds, RenderOctreeChunkSize, RenderOctreeDepth);
}

FVoxelGeneratorInit FVoxelRuntimeSettings::GetGeneratorInit() const
{
	// See AVoxelWorld::GetGeneratorInit
	
	return FVoxelGeneratorInit(
		VoxelSize,
		FVoxelUtilities::GetSizeFromDepth(RenderOctreeChunkSize, RenderOctreeDepth),
		RenderType,
		MaterialConfig,
		MaterialCollection.Get(),
		VoxelWorld.Get());
}

void FVoxelRuntimeSettings::SetupComponent(USceneComponent& Component) const
{
	Component.SetupAttachment(AttachRootComponent.Get(), NAME_None);
	
	Component.SetUsingAbsoluteLocation(bUseAbsoluteTransforms);
	Component.SetUsingAbsoluteRotation(bUseAbsoluteTransforms);
	Component.SetUsingAbsoluteScale(bUseAbsoluteTransforms);

	Component.SetRelativeTransform(FTransform::Identity);
}

void FVoxelRuntimeSettings::SetComponentPosition(USceneComponent& Component, const FIntVector& Position, bool bScaleByVoxelSize) const
{
	const FVoxelDoubleVector RelativePosition = FVoxelDoubleVector(Position) * VoxelSize;
	const FVoxelDoubleVector RelativeScale = bScaleByVoxelSize ? FVector(VoxelSize) : FVector::OneVector;

	FVoxelDoubleTransform Transform;
	if (bUseAbsoluteTransforms)
	{
		const AVoxelWorld* VoxelWorldPtr = VoxelWorld.Get();
		if (!ensureVoxelSlow(VoxelWorldPtr))
		{
			return;
		}

		const FVoxelDoubleTransform LocalTransform(FVoxelDoubleQuat::Identity, RelativePosition, RelativeScale);
		const FVoxelDoubleTransform LocalToGlobalTransform = VoxelWorldPtr->GetVoxelTransform();
		const FVoxelDoubleTransform GlobalTransform = LocalTransform * LocalToGlobalTransform;

		Transform = GlobalTransform;
	}
	else
	{
		Transform = FVoxelDoubleTransform(FVoxelQuat::Identity, RelativePosition, RelativeScale);
	}

	Component.SetRelativeLocation_Direct(Transform.GetTranslation());
	Component.SetRelativeRotation_Direct(Transform.GetRotation());
	Component.SetRelativeScale3D_Direct(Transform.GetScale3D());
	Component.UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimeDynamicSettings::FVoxelRuntimeDynamicSettings()
{
	SetFromRuntime(*GetDefault<AVoxelRuntimeActor>());
}

void FVoxelRuntimeDynamicSettings::SetFromRuntime(const AVoxelRuntimeActor& Runtime)
{
#define SET(Name) Name = Runtime.Name
	
	SET(MinLOD);
	SET(MaxLOD);
	SET(ChunksCullingLOD);
	
	SET(InvokerDistanceThreshold);
	SET(bRenderWorld);

	SET(bEnableCollisions);
	SET(bComputeVisibleChunksCollisions);
	SET(VisibleChunksCollisionsMaxLOD);

	SET(bEnableNavmesh);
	SET(bComputeVisibleChunksNavmesh);
	SET(VisibleChunksNavmeshMaxLOD);

	TArray<UVoxelMaterialCollectionBase*> MaterialCollectionsToInitialize;
	for (int32 LOD = 0; LOD < 32; LOD++)
	{
		auto& Settings = MaterialSettings[LOD];

		// Copy materials
		Settings.Material = nullptr;
		for (auto& It : Runtime.LODMaterials)
		{
			if (It.StartLOD <= LOD && LOD <= It.EndLOD)
			{
				if (Settings.Material.IsValid())
				{
					FVoxelMessages::Warning(FString::Printf(TEXT("Multiple materials are assigned to LOD %d!"), LOD), &Runtime);
				}
				Settings.Material = It.Material;
			}
		}
		if (!Settings.Material.IsValid())
		{
			Settings.Material = Runtime.VoxelMaterial;
		}

		// Copy material collection
		Settings.MaterialCollection = nullptr;
		for (auto& It : Runtime.LODMaterialCollections)
		{
			if (It.StartLOD <= LOD && LOD <= It.EndLOD)
			{
				if (Settings.MaterialCollection.IsValid())
				{
					FVoxelMessages::Warning(FString::Printf(TEXT("Multiple material collections are assigned to LOD %d!"), LOD), &Runtime);
				}
				Settings.MaterialCollection = It.MaterialCollection;
			}
		}
		if (!Settings.MaterialCollection.IsValid())
		{
			Settings.MaterialCollection = Runtime.MaterialCollection;
		}

		// Set MaxMaterialIndices
		if (auto* Collection = Settings.MaterialCollection.Get())
		{
			Settings.bEnableCubicFaces = Collection->EnableCubicFaces();
			Settings.MaxMaterialIndices.Set(FMath::Max(Collection->GetMaxMaterialIndices(), 1));
			MaterialCollectionsToInitialize.AddUnique(Collection);
		}
		else
		{
			Settings.bEnableCubicFaces = false;
			Settings.MaxMaterialIndices.Set(1);
		}
	}

	// Initialize all used collections
	for (auto* Collection : MaterialCollectionsToInitialize)
	{
		Collection->InitializeCollection();
	}

	Fixup();
}

void FVoxelRuntimeDynamicSettings::ConfigurePreview()
{
	bEnableCollisions = true;
	bComputeVisibleChunksCollisions = true;
	VisibleChunksCollisionsMaxLOD = 32;

	// bEnableNavmesh is needed for path previews in editor, so don't disable it

	Fixup();
}

void FVoxelRuntimeDynamicSettings::Fixup()
{
	MinLOD = FVoxelUtilities::ClampDepth(MinLOD);
	MaxLOD = FVoxelUtilities::ClampDepth(MaxLOD);
	
	ChunksCullingLOD = FVoxelUtilities::ClampDepth(ChunksCullingLOD);
	VisibleChunksCollisionsMaxLOD = FVoxelUtilities::ClampDepth(VisibleChunksCollisionsMaxLOD);
	VisibleChunksNavmeshMaxLOD = FVoxelUtilities::ClampDepth(VisibleChunksNavmeshMaxLOD);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimeData::FVoxelRuntimeData()
	: InvokersPositionsForPriorities(MakeVoxelShared<FInvokerPositionsArray>(32))
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelSharedRef<FVoxelRuntime> FVoxelRuntime::Create(FVoxelRuntimeSettings Settings)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	Settings.Fixup();
	
	const auto Runtime = FVoxelUtilities::MakeGameThreadDeleterPtr<FVoxelRuntime>();
	{
		const TGuardValue<bool> InitGuard(Runtime->bIsInit, true);

		// Add static subsystems
		{
			TArray<UClass*> Classes;
			GetDerivedClasses(UVoxelStaticSubsystemProxy::StaticClass(), Classes);

			for (auto* Class : Classes)
			{
				if (!Class->HasAnyClassFlags(CLASS_Abstract))
				{
					Runtime->AddSubsystem(Class, Settings);
				}
			}
		}

		// Add dynamic subsystems
		{
			const auto AddMainSubsystem = [&](UClass* Class, UClass* DefaultClass)
			{
				if (!Class || Class->HasAnyClassFlags(CLASS_Abstract))
				{
					Class = DefaultClass;
				}

				Runtime->AddSubsystem(Class, Settings);
			};

			AddMainSubsystem(Settings.RendererSubsystem, UVoxelDefaultRendererSubsystemProxy::StaticClass());
			AddMainSubsystem(Settings.LODSubsystem, UVoxelDefaultLODSubsystemProxy::StaticClass());
		}

		for (auto& Subsystem : Runtime->AllSubsystems)
		{
			ensure(Runtime->SubsystemsBeingInitialized.Num() == 0);
			Runtime->InitializeSubsystem(Subsystem);
			ensure(Runtime->SubsystemsBeingInitialized.Num() == 0);
		}
		ensure(Runtime->InitializedSubsystems.Num() == Runtime->AllSubsystems.Num());
		// Note: we can't clear InitializedSubsystems, due to RecreateSubsystem
	}
	
	for (auto& Subsystem : Runtime->AllSubsystems)
	{
		Subsystem->PostCreate(nullptr);
	}
	
	return Runtime;
}

FVoxelRuntime::~FVoxelRuntime()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(bIsDestroyed))
	{
		Destroy();
	}
}

void FVoxelRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	ensure(!bIsDestroyed);
	bIsDestroyed = true;
	
	for (auto& Subsystem : AllSubsystems)
	{
		ensure(Subsystem->State == IVoxelSubsystem::EState::Before_Destroy);
		Subsystem->Destroy();
		ensure(Subsystem->State == IVoxelSubsystem::EState::Destroy);
	}
}

void FVoxelRuntime::RecreateSubsystems(EVoxelSubsystemFlags Flags, FVoxelRuntimeSettings Settings)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	// No need to lock anything when on the game thread
	
	Settings.Fixup();

	const auto AllSubsystemsCopy = AllSubsystems;
	for (const TVoxelSharedPtr<IVoxelSubsystem>& Subsystem : AllSubsystemsCopy)
	{
		if (EnumHasAnyFlags(Subsystem->GetFlags(), Flags))
		{
			RecreateSubsystem(Subsystem, Settings);
		}
	}
}

void FVoxelRuntime::InitializeSubsystem(const TVoxelSharedPtr<IVoxelSubsystem>& Subsystem)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(bIsInit);
	check(Subsystem);
	
	if (InitializedSubsystems.Contains(Subsystem))
	{
		return;
	}
	
	checkf(!SubsystemsBeingInitialized.Contains(Subsystem), TEXT("Recursive dependencies!"));
	SubsystemsBeingInitialized.Add(Subsystem);
	
	ensure(Subsystem->State == IVoxelSubsystem::EState::Before_Create);
	Subsystem->Create();
	ensure(Subsystem->State == IVoxelSubsystem::EState::Create);

	SubsystemsBeingInitialized.Remove(Subsystem);
	InitializedSubsystems.Add(Subsystem);
}

void FVoxelRuntime::RecreateSubsystem(TVoxelSharedPtr<IVoxelSubsystem> OldSubsystem, const FVoxelRuntimeSettings& Settings)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	
	if (!OldSubsystem)
	{
		// Allowed: useful to recreate subsystems whose class didn't want to be created
		return;
	}
	
	FScopeLock Lock(&RecreateSection);
	
	OldSubsystem->Destroy();
	AllSubsystems.Remove(OldSubsystem);

	for (auto It = SubsystemsMap.CreateIterator(); It; ++It)
	{
		if (It.Value() == OldSubsystem)
		{
			It.RemoveCurrent();
		}
	}
	
	UClass* Class = OldSubsystem->GetProxyClass();

	TVoxelSharedPtr<IVoxelSubsystem> NewSubsystem;
	{
		check(!bIsInit);
		const TGuardValue<bool> InitGuard(bIsInit, true);

		NewSubsystem = AddSubsystem(Class, Settings);
		if (!NewSubsystem)
		{
			// ShouldCreateSubsystem returned false
			return;
		}

		ensure(SubsystemsBeingInitialized.Num() == 0);
		InitializeSubsystem(NewSubsystem);
		ensure(SubsystemsBeingInitialized.Num() == 0);
	}
	NewSubsystem->PostCreate(OldSubsystem.Get());
}

TVoxelSharedPtr<IVoxelSubsystem> FVoxelRuntime::AddSubsystem(UClass* Class, const FVoxelRuntimeSettings& Settings)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(Class && !Class->HasAnyClassFlags(CLASS_Abstract));

	const bool bStaticSubsystem = Class->IsChildOf<UVoxelStaticSubsystemProxy>();
	check(bStaticSubsystem || Class->IsChildOf<UVoxelDynamicSubsystemProxy>());
	
	if (bStaticSubsystem && !Class->GetDefaultObject<UVoxelStaticSubsystemProxy>()->ShouldCreateSubsystem(*this, Settings))
	{
		return nullptr;
	}

	const TVoxelSharedRef<IVoxelSubsystem> Subsystem = Class->GetDefaultObject<UVoxelSubsystemProxy>()->GetSubsystem(*this, Settings);
	AllSubsystems.Add(Subsystem);

	// Add to the whole hierarchy so that we can query using parent classes
	for (UClass* ClassIt = Class;
		ClassIt != UVoxelStaticSubsystemProxy::StaticClass() &&
		ClassIt != UVoxelDynamicSubsystemProxy::StaticClass();
		ClassIt = ClassIt->GetSuperClass())
	{
		ensure(!SubsystemsMap.Contains(ClassIt));
		SubsystemsMap.Add(ClassIt, Subsystem);
	}

	return Subsystem;
}