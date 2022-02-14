// Copyright 2021 Phyronnaz

#include "VoxelPlaceableItems/Actors/VoxelAssetActor.h"
#include "VoxelAssets/VoxelDataAsset.h"
#include "VoxelAssets/VoxelDataAssetData.inl"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelDebug/VoxelDebugManager.h"
#include "VoxelRender/IVoxelRenderer.h"
#include "VoxelRender/IVoxelLODManager.h"
#include "VoxelRender/VoxelTexturePool.h"
#include "VoxelRender/LODManager/VoxelFixedResolutionLODManager.h"
#include "VoxelRender/VoxelProceduralMeshComponent.h"
#include "VoxelRender/Renderers/VoxelDefaultRenderer.h"
#include "VoxelGenerators/VoxelEmptyGenerator.h"
#include "VoxelWorld.h"
#include "VoxelWorldRootComponent.h"
#include "VoxelPool.h"
#include "VoxelMessages.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"

#include "Components/BoxComponent.h"

AVoxelAssetActor::AVoxelAssetActor()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITOR
	PrimitiveComponent = CreateDefaultSubobject<UAssetActorPrimitiveComponent>(TEXT("PrimitiveComponent"));
	PrimitiveComponent->SetupAttachment(Root);

	SetActorEnableCollision(true); // To place items on top of it

	Box = CreateDefaultSubobject<UBoxComponent>("Box");
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetupAttachment(Root);

	PrimaryActorTick.bCanEverTick = true;
#endif
}

void AVoxelAssetActor::AddItemToWorld(AVoxelWorld* World)
{
	check(World);

	if (bSpawnNewVoxelWorld)
	{
		return;
	}
	if (World->GetPlayType() != EVoxelPlayType::Game)
	{
		return;
	}
	if (!Generator.IsValid())
	{
		FVoxelMessages::Error("Invalid generator", this);
		return;
	}

	AddItemToData(World, &World->GetSubsystemChecked<FVoxelData>());
}

int32 AVoxelAssetActor::GetPriority() const
{
	return Priority;
}

FVoxelIntBox AVoxelAssetActor::AddItemToData(
	AVoxelWorld* VoxelWorld,
	FVoxelData* VoxelWorldData) const
{
	VOXEL_FUNCTION_COUNTER();
	
	auto Transform = GetTransform() * VoxelWorld->GetTransform().Inverse();
	Transform.ScaleTranslation(1.f / VoxelWorld->VoxelSize);

	FVoxelIntBox WorldBounds;
	if (bOverrideAssetBounds)
	{
		// Might be one-off error there
		WorldBounds = AssetBounds.Translate(FVoxelUtilities::FloorToInt(Transform.GetTranslation()));
	}
	else
	{
		if (auto* GeneratorWithBounds = Cast<UVoxelTransformableGeneratorWithBounds>(Generator.GetObject()))
		{
			WorldBounds = GeneratorWithBounds->GetBounds().ApplyTransform(Transform);
		}
		else
		{
			FVoxelMessages::Error(
				"Voxel Asset Actor: AssetBounds are not overriden, and cannot deduce them from the generator\n"
				"You need to tick the checkbox next to Asset Bounds on the asset actor",
				this);
			WorldBounds = FVoxelIntBox(-25, 25).Translate(FVoxelUtilities::FloorToInt(Transform.GetTranslation()));
		}

		// Small hack to update the asset bounds from the world bounds when bOverrideAssetBounds = false
		const_cast<AVoxelAssetActor*>(this)->AssetBounds = WorldBounds.Translate(-FVoxelUtilities::FloorToInt(Transform.GetTranslation()));
	}

	if (!VoxelWorldData || !ensure(WorldBounds.IsValid()))
	{
		return WorldBounds;
	}

	auto AssetInstance = Generator.GetInstance();
	AssetInstance->Init(VoxelWorld->GetGeneratorInit());
	
	if (bImportAsReference)
	{
		FVoxelWriteScopeLock Lock(*VoxelWorldData, WorldBounds, FUNCTION_FNAME);
		VoxelWorldData->AddItem<FVoxelAssetItem>(
			AssetInstance,
			WorldBounds,
			Transform,
			Priority);
	}
	else
	{
		if (WorldBounds.Count() > 1e8)
		{
			FVoxelMessages::Error(
				"Voxel Asset Actor: importing would affect more than 100 000 000 voxels. Please tick ImportAsReference instead.",
				this);
		}
		else
		{
			FVoxelWriteScopeLock Lock(*VoxelWorldData, WorldBounds, FUNCTION_FNAME);
			UVoxelAssetTools::ImportAssetImpl(
				*VoxelWorldData,
				WorldBounds,
				Transform,
				*AssetInstance,
				bSubtractiveAsset,
				MergeMode,
				EVoxelMaterialMask::All); // TODO: expose material mask?
		}
	}

	return WorldBounds;
}

void AVoxelAssetActor::ClampTransform()
{
	const bool bForceRound = PreviewWorld->RenderType == EVoxelRenderType::Cubic;
	
	if (bRoundAssetPosition || bForceRound)
	{
		const FVector WorldLocation = PreviewWorld->GetActorLocation();
		const float VoxelSize = PreviewWorld->VoxelSize;

		FVector Position = GetActorLocation();
		Position -= WorldLocation;
		Position /= VoxelSize;

		Position.X = FMath::RoundToInt(Position.X);
		Position.Y = FMath::RoundToInt(Position.Y);
		Position.Z = FMath::RoundToInt(Position.Z);

		Position *= VoxelSize;
		Position += WorldLocation;

		SetActorLocation(Position);
	}
	
	if (bRoundAssetRotation || bForceRound)
	{
		const FRotator WorldRotation = PreviewWorld->GetActorRotation();
		FRotator Rotation = GetActorRotation();

		Rotation = (FRotationMatrix(Rotation) * FRotationMatrix(WorldRotation).Inverse()).Rotator();

		Rotation.Pitch = FMath::RoundToInt(Rotation.Pitch / 90) * 90;
		Rotation.Yaw = FMath::RoundToInt(Rotation.Yaw / 90) * 90;
		Rotation.Roll = FMath::RoundToInt(Rotation.Roll / 90) * 90;
		
		Rotation = (FRotationMatrix(Rotation) * FRotationMatrix(WorldRotation)).Rotator();
		
		SetActorRotation(Rotation);
	}
}

#if WITH_EDITOR
void AVoxelAssetActor::UpdatePreview()
{
	if (!PreviewWorld) return;
	if (!Generator.IsValid()) return;

	if (IsPreviewCreated())
	{
		DestroyPreview();
		CreatePreview();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelAssetActor::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnNewVoxelWorld && PreviewWorld)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.bDeferConstruction = true;
		SpawnParameters.Template = PreviewWorld;
		auto* VoxelWorld = GetWorld()->SpawnActor<AVoxelWorld>(PreviewWorld->GetClass(), SpawnParameters);
		if (!ensure(VoxelWorld)) return;
		
		// Attach to ourselves
		VoxelWorld->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		VoxelWorld->SetActorRelativeTransform(FTransform::Identity);

		// Setup physics
		VoxelWorld->GetWorldRoot().BodyInstance.SetInstanceSimulatePhysics(bSimulatePhysics);
		if (bSimulatePhysics && VoxelWorld->CollisionTraceFlag == CTF_UseComplexAsSimple)
		{
			VoxelWorld->CollisionTraceFlag = CTF_UseSimpleAndComplex;
		}

		// Setup LOD
		const FVoxelIntBox ItemBounds = AddItemToData(VoxelWorld, nullptr);
		// TODO Set RenderOctreeChunkSize?
		VoxelWorld->SetRenderOctreeDepth(FVoxelUtilities::GetOctreeDepthContainingBounds(MESHER_CHUNK_SIZE, ItemBounds));
		VoxelWorld->MaxLOD = 0;
		VoxelWorld->bConstantLOD = true;

		// Finish spawning & create world
		VoxelWorld->bCreateWorldAutomatically = false;
		VoxelWorld->FinishSpawning({}, true);
		VoxelWorld->CreateWorld();

		AddItemToData(VoxelWorld, &VoxelWorld->GetSubsystemChecked<FVoxelData>());
	}

#if WITH_EDITOR
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	PrimaryActorTick.SetTickFunctionEnable(false);
#endif
}

#if WITH_EDITOR
void AVoxelAssetActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (IsPreviewCreated())
	{
		DestroyPreview();
	}
}

void AVoxelAssetActor::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();
	
	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		// Editor preview still ticks
		return;
	}
	if (PreviewWorld)
	{
		if (Generator.IsValid() && !IsPreviewCreated())
		{
			CreatePreview();
		}
		if (!PreviewWorld->OnPropertyChanged.IsBoundToObject(this))
		{
			PreviewWorld->OnPropertyChanged.AddUObject(this, &AVoxelAssetActor::UpdatePreview);
		}
		
		UpdateBox();
	}
}

void AVoxelAssetActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PreviewWorld && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		ClampTransform();
	}
	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		const auto Name = PropertyChangedEvent.MemberProperty->GetFName();
		if (Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, Generator) ||
			Name == GET_MEMBER_NAME_STATIC(FVoxelTransformableGeneratorPicker, Type) ||
			Name == GET_MEMBER_NAME_STATIC(FVoxelTransformableGeneratorPicker, Class) ||
			Name == GET_MEMBER_NAME_STATIC(FVoxelTransformableGeneratorPicker, Object) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, bOverrideAssetBounds) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, AssetBounds) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, PreviewLOD) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, bSubtractiveAsset) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, bImportAsReference) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, PreviewWorld) ||
			Name == STATIC_FNAME("RelativeScale3D") ||
			Name == STATIC_FNAME("RelativeRotation"))
		{
			UpdatePreview();
		}
	}
}

void AVoxelAssetActor::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (PreviewWorld && Generator.IsValid())
	{
		if (UpdateType == EVoxelAssetActorPreviewUpdateType::RealTime)
		{
			UpdatePreview();
		}
		if (bFinished)
		{
			ClampTransform();
			if (UpdateType == EVoxelAssetActorPreviewUpdateType::EndOfMove)
			{
				UpdatePreview();
			}
		}
	}
}

bool AVoxelAssetActor::CanEditChange(const FProperty* InProperty) const
{
	if (InProperty)
	{
		const FName Name = InProperty->GetFName();
		if (Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, bRoundAssetPosition) ||
			Name == GET_MEMBER_NAME_STATIC(AVoxelAssetActor, bRoundAssetRotation))
		{
			if (PreviewWorld && PreviewWorld->RenderType == EVoxelRenderType::Cubic)
			{
				return false;
			}
		}
	}

	return Super::CanEditChange(InProperty);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
bool AVoxelAssetActor::IsPreviewCreated() const
{
	return Runtime.IsValid();
}

void AVoxelAssetActor::CreatePreview()
{
	BindEditorDelegates(this);

	if (!ensure(PreviewWorld)) return;
	if (!ensure(Generator.IsValid())) return;
	if (!ensure(!IsPreviewCreated())) return;

	PrimitiveComponent->SetWorldTransform(PreviewWorld->GetTransform());
	const FVoxelIntBox Bounds = AddItemToData(PreviewWorld, nullptr);

	TVoxelSharedPtr<FVoxelData> Data;
	{
		bool bIsGeneratorSubtractive = bSubtractiveAsset;
		if (bImportAsReference)
		{
			if (auto* DataAsset = Cast<UVoxelDataAsset>(Generator.GetObject()))
			{
				bIsGeneratorSubtractive = DataAsset->bSubtractiveAsset;
			}
			else
			{
				bIsGeneratorSubtractive = false;
			}
		}
		auto EmptyGenerator = MakeVoxelShared<FVoxelEmptyGeneratorInstance>(bIsGeneratorSubtractive ? -1 : 1);
		EmptyGenerator->Init(PreviewWorld->GetGeneratorInit());
		Data = FVoxelData::Create(
			FVoxelDataSettings(
				Bounds,
				EmptyGenerator,
				false,
				false));

		const auto RealMergeMode = MergeMode;
		MergeMode = EVoxelAssetMergeMode::AllValuesAndAllMaterials;
		AddItemToData(PreviewWorld, Data.Get());
		MergeMode = RealMergeMode;
	}

	// Needed for some reason to place stuff
	PrimitiveComponent->BodyInstance.CopyRuntimeBodyInstancePropertiesFrom(&PreviewWorld->GetWorldRoot().BodyInstance);
	PrimitiveComponent->BodyInstance.SetObjectType(PreviewWorld->GetWorldRoot().BodyInstance.GetObjectType());

	FVoxelRuntimeSettings Settings;
	Settings.SetFromRuntime(*PreviewWorld);
	Settings.ConfigurePreview();
	Settings.Owner = this;
	Settings.ComponentsOwner = this;
	Settings.AttachRootComponent = PrimitiveComponent;
	Settings.DataOverride = Data;
	Settings.bUseCustomWorldBounds = true;
	Settings.CustomWorldBounds = Bounds;
	Settings.bDisableDebugManager = true;

	// Aggressive merge settings
	Settings.bMergeChunks = true;
	Settings.MergedChunksClusterSize = 8;

	// We do want collision to be able to place items on top of asset actors, but only complex one is needed
	Settings.CollisionTraceFlag = CTF_UseComplexAsSimple;
	
	Settings.LODSubsystem = FVoxelFixedResolutionLODManager::StaticClass();
	
	Runtime = FVoxelRuntime::Create(Settings);
	Runtime->DynamicSettings->SetFromRuntime(*PreviewWorld);
	Runtime->DynamicSettings->ConfigurePreview();

	auto& LODManager = Runtime->GetSubsystemChecked<FVoxelFixedResolutionLODManager>();
	while (ensure(PreviewLOD < 24) && !LODManager.Initialize(
		FVoxelUtilities::ClampDepth(Settings.RenderOctreeChunkSize, PreviewLOD),
		MaxPreviewChunks,
		true,
		true,
		false))
	{
		PreviewLOD++;
	}
}

void AVoxelAssetActor::DestroyPreview()
{
	if (!ensure(IsPreviewCreated())) return;

	Runtime->Destroy();
	Runtime.Reset();

	auto Components = GetComponents(); // need a copy as we are modifying it when destroying comps
	for (auto& Component : Components)
	{
		if (Component && Component->HasAnyFlags(RF_Transient) && Component->IsA<UVoxelProceduralMeshComponent>())
		{
			Component->DestroyComponent();
		}
	}
}

void AVoxelAssetActor::UpdateBox()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (!ensure(PreviewWorld)) return;

	const FVoxelIntBox Bounds = AddItemToData(PreviewWorld, nullptr);

	Box->SetWorldTransform(PreviewWorld->GetTransform());
	Box->SetBoxExtent(FVector(Bounds.Size()) / 2 * PreviewWorld->VoxelSize * PreviewWorld->GetActorScale3D());
	Box->SetWorldLocation(PreviewWorld->LocalToGlobalFloat(Bounds.GetCenter()));
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void AVoxelAssetActor::OnPrepareToCleanseEditorObject(UObject* Object)
{
	DestroyPreview();
}
#endif