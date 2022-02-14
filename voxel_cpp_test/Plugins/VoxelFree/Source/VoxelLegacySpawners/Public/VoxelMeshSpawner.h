// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelBasicSpawner.h"
#include "VoxelSpawnerActorSettings.h"
#include "VoxelInstancedMeshSettings.h"
#include "VoxelMeshSpawner.generated.h"

UENUM()
enum class EVoxelMeshSpawnerInstanceRandom : uint8
{
	// Random number
	// Use GetVoxelSpawnerActorInstanceRandom to get it
	// Will have the same value in the spawned actor as in the instance
	Random,
	// Get the voxel material in the shader
	// Use GetVoxelMaterialFromInstanceRandom
	VoxelMaterial,
	// Get a voxel graph output color in the shader
	// Use GetColorFromInstanceRandom
	ColorOutput
};

UCLASS(Abstract)
class VOXELLEGACYSPAWNERS_API UVoxelMeshSpawnerBase : public UVoxelBasicSpawner
{
	GENERATED_BODY()

public:
	// What to send through InstanceRandom
	// Check enum values tooltips
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
	EVoxelMeshSpawnerInstanceRandom InstanceRandom = EVoxelMeshSpawnerInstanceRandom::Random;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
	FName ColorOutputName;
	
	// Actor to spawn to replace the instanced mesh. After spawn, the SetStaticMesh event will be called on the actor with Mesh as argument
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Settings", meta = (ShowOnlyInnerProperties))
	FVoxelSpawnerActorSettings ActorSettings;

	// Will always spawn an actor instead of an instanced mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Actor Settings")
	bool bAlwaysSpawnActor = false;

public:
	UPROPERTY(EditAnywhere, Category = "Instance Settings", meta = (ShowOnlyInnerProperties))
	FVoxelInstancedMeshSettings InstancedMeshSettings;
	
public:
	// In local space. Increase this if your foliage is enabling physics too soon. In cm
	UPROPERTY(EditAnywhere, Category = "Placement - Offset")
	FVector FloatingDetectionOffset = FVector(0, 0, -10);
};

UCLASS()
class VOXELLEGACYSPAWNERS_API UVoxelMeshSpawner : public UVoxelMeshSpawnerBase
{
	GENERATED_BODY()

public:
	// Mesh to spawn. Can be left to null if AlwaysSpawnActor is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
	UStaticMesh* Mesh = nullptr;

	// Per mesh section
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "General Settings")
	TMap<int32, UMaterialInterface*> MaterialsOverrides;
};

UCLASS()
class VOXELLEGACYSPAWNERS_API UVoxelMeshSpawnerGroup : public UVoxelMeshSpawnerBase
{
	GENERATED_BODY()

public:
	// Meshes to spawn. Can be left to null if AlwaysSpawnActor is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
	TArray<UStaticMesh*> Meshes;
};