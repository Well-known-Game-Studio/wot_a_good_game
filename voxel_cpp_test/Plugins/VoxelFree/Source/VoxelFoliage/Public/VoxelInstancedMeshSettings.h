// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelInterval.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "VoxelFoliageActor.h"
#include "VoxelInstancedMeshSettings.generated.h"

class UStaticMesh;
class UVoxelHierarchicalInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelInstancedMeshSettings
{
	GENERATED_BODY()

	FVoxelInstancedMeshSettings();
	
public:
	// Distance from camera at which each instance begins/completely to fade out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance Settings")
	FVoxelInt32Interval CullDistance = { 100000, 200000 };

	/** Controls whether the foliage should cast a shadow or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance Settings")
	bool bCastShadow = true;

	/** Controls whether the foliage should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", meta=(EditCondition="bCastShadow"))
	bool bAffectDynamicIndirectLighting = false;

	/** Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", meta=(EditCondition="bCastShadow"))
	bool bAffectDistanceFieldLighting = false;

	/** Whether this foliage should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", meta=(EditCondition="bCastShadow"))
	bool bCastShadowAsTwoSided = false;

	/** Whether the foliage receives decals. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings")
	bool bReceivesDecals = true;

	/**
	 * If enabled, foliage will render a pre-pass which allows it to occlude other primitives, and also allows 
	 * it to correctly receive DBuffer decals. Enabling this setting may have a negative performance impact.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings")
	bool bUseAsOccluder = false;

	/** Custom collision for foliage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance Settings", meta = (ShowOnlyInnerProperties))
	FBodyInstance BodyInstance;

	/** Force navmesh */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Instance Settings")
	TEnumAsByte<EHasCustomNavigableGeometry::Type> CustomNavigableGeometry = {};

	/**
	 * Lighting channels that placed foliage will be assigned. Lights with matching channels will affect the foliage.
	 * These channels only apply to opaque materials, direct lighting, and dynamic lighting and shadowing.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings")
	FLightingChannels LightingChannels{};

	/** If true, the foliage will be rendered in the CustomDepth pass (usually used for outlines) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", meta=(DisplayName = "Render CustomDepth Pass"))
	bool bRenderCustomDepth = false;

	/** Optionally write this 0-255 value to the stencil buffer in CustomDepth pass (Requires project setting or r.CustomDepth == 3) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings",  meta=(UIMin = "0", UIMax = "255", editcondition = "bRenderCustomDepth", DisplayName = "CustomDepth Stencil Value"))
	int32 CustomDepthStencilValue = 0;

	// If more instances are added before BuildDelay seconds elapsed, the tree build is queued
	// This is useful to avoid spending lots of time building the tree for nothing.
	// However, it can lead to delays in foliage spawning.
	// To disable this feature entirely, set it to 0
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", meta = (ClampMin = 0, DisplayName = "Culling Tree Build Delay"))
	float BuildDelay = 0.1;

	// If you want to edit the HISM properties create a BP inheriting from HierarchicalInstancedStaticMeshComponent and set it here
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Instance Settings", AdvancedDisplay, meta = (DisplayName = "HISM Template"))
	TSubclassOf<UVoxelHierarchicalInstancedStaticMeshComponent> HISMTemplate;
};

USTRUCT(BlueprintType)
struct FVoxelInstancedMeshKey
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	TArray<UMaterialInterface*> Materials;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	TSubclassOf<AVoxelFoliageActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	FVoxelInstancedMeshSettings InstanceSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	int32 NumCustomDataChannels = 0;
};

struct FVoxelInstancedMeshWeakKey
{
	TWeakObjectPtr<UStaticMesh> Mesh;
	TSubclassOf<AVoxelFoliageActor> ActorClass;
	TArray<TWeakObjectPtr<UMaterialInterface>> Materials;
	FVoxelInstancedMeshSettings InstanceSettings;
	int32 NumCustomDataChannels = 0;

	FVoxelInstancedMeshWeakKey() = default;
	FVoxelInstancedMeshWeakKey(const FVoxelInstancedMeshKey& Key)
		: Mesh(Key.Mesh)
		, ActorClass(Key.ActorClass)
		, Materials(Key.Materials)
		, InstanceSettings(Key.InstanceSettings)
		, NumCustomDataChannels(Key.NumCustomDataChannels)
	{
	}
	operator FVoxelInstancedMeshKey() const
	{
		FVoxelInstancedMeshKey Result;
		Result.Mesh = Mesh.Get();
		Result.ActorClass = ActorClass;
		for (const TWeakObjectPtr<UMaterialInterface>& Material : Materials)
		{
			Result.Materials.Add(Material.Get());
		}
		Result.InstanceSettings = InstanceSettings;
		Result.NumCustomDataChannels = NumCustomDataChannels;
		return Result;
	}
};

VOXELFOLIAGE_API bool operator==(const FVoxelInstancedMeshWeakKey& A, const FVoxelInstancedMeshWeakKey& B);
VOXELFOLIAGE_API uint32 GetTypeHash(const FVoxelInstancedMeshWeakKey& Settings);