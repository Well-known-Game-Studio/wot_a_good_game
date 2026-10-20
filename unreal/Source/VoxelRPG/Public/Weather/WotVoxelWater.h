#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotVoxelWater.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * Hand-made voxel water life, in the spirit of the fireflies and the voxel weather: one instanced cube mesh of little
 * pale flecks. At BeginPlay it finds every water voxel in the level (instanced meshes whose material is the voxel water)
 * and works out, per cell, which way the water runs (the local axis of the neighbouring cells, signed by Flow). Then:
 *  - flow lines: short flat streaks drifting downstream over the surface, fading in and out;
 *  - falls: where a water cell has a neighbour far below it, flecks drop over the edge;
 *  - ripples: a ring of flecks spreads out from anyone standing or wading in the water (faster rings when they move).
 */
UCLASS()
class VOXELRPG_API AWotVoxelWater : public AActor
{
	GENERATED_BODY()
public:
	AWotVoxelWater();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UInstancedStaticMeshComponent> Flecks;

	/** Downstream hint, world space (only the direction matters): resolves which way the local axis of each cell points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FVector Flow = FVector(1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	bool bEnabled = true;

	/** Streaks per water cell kept alive (0.1 = one streak for every ten cells). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	float StreakDensity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	float StreakSpeed = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	FVector StreakSize = FVector(90.0f, 11.0f, 5.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	float RippleRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	float RippleSeconds = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	int32 MaxFlecks = 1400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	TObjectPtr<UStaticMesh> CubeMesh;

	/** Fleck colour: a palette instance; the pale one by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Look")
	TObjectPtr<UMaterialInterface> Material;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Water cells found in the level (for the console / tests). */
	int32 NumCells() const { return Cells.Num(); }
	/** Live fleck counts and where the characters' feet are relative to the water (wot.Water.Status). */
	FString Describe() const;

private:
	struct FCell { FVector2D Centre; float HalfX; float HalfY; float Top; FVector2D Axis; int32 FallTo; };
	struct FFleck { FVector Pos; FVector Vel; float Age; float Life; FVector Size; uint8 Kind; FVector2D Ring; float RingSpeed; };
	struct FWader { TWeakObjectPtr<AActor> Actor; float Timer; };

	TArray<FCell> Cells;
	TMap<FIntPoint, TArray<int32>> Grid;   // 200 cm buckets of cell indices
	TArray<FFleck> Live;
	TArray<FWader> Waders;
	TArray<int32> FallCells;
	float StreakAccumulator = 0.0f;
	float FallAccumulator = 0.0f;

	void GatherCells();
	int32 CellAt(const FVector& P) const;
	void SpawnStreak();
	void SpawnFall();
	void SpawnRipple(const FVector& Centre, float Strength);
	void WriteInstances();
};
