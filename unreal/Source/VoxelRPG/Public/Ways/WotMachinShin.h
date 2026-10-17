#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotMachinShin.generated.h"

class UInstancedStaticMeshComponent;
class USphereComponent;
class UAudioComponent;

/**
 * Machin Shin, the Black Wind of the Ways: a murmuration of dark stone shards
 * that drifts along the bridges and hunts anything it hears. Contact drains
 * health and throws the victim back; it cannot be fought, only outrun.
 *
 * Built from an instanced cube mesh animated in C++ (curl-noise flock around a
 * moving centre) so it needs no Niagara asset. Patrol points are the island
 * centres handed in by the level generator (PatrolPoints); with none set it
 * wanders around its spawn location.
 */
UCLASS()
class VOXELRPG_API AWotMachinShin : public AActor
{
	GENERATED_BODY()

public:
	AWotMachinShin();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Machin Shin")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Machin Shin")
	TObjectPtr<UInstancedStaticMeshComponent> Shards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Machin Shin")
	TObjectPtr<USphereComponent> Reach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	int32 ShardCount = 600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float SwarmRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float ShardSize = 22.0f;

	/** Patrol speed along the bridges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float WanderSpeed = 220.0f;

	/** Speed when it has heard the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float HuntSpeed = 560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float HearingRange = 2600.0f;

	/** It loses the player beyond this distance and drifts back to the bridges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float LoseRange = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float DamagePerSecond = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	float Knockback = 900.0f;

	/** Island centres (world space) it patrols between. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	TSoftObjectPtr<UStaticMesh> ShardMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Machin Shin")
	TSoftObjectPtr<UMaterialInterface> ShardMaterial;

	UFUNCTION(BlueprintCallable, Category = "Machin Shin")
	bool IsHunting() const { return bHunting; }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void RebuildShards();

	bool bHunting = false;
	int32 PatrolIndex = 0;
	FVector Target = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Time = 0.0f;
	float DamageAccumulator = 0.0f;
	TArray<FVector> ShardOffsets;
	TArray<float> ShardPhase;
};
