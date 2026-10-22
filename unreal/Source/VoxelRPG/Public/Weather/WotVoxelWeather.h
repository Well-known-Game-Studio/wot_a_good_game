#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WotVoxelWeather.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EWotWeatherKind : uint8
{
	Rain,
	Snow,
	Ash,
	Sand,
	Leaves
};

/**
 * Hand-made voxel weather, in the spirit of the fireflies: little cubes falling, drifting or blowing around the player,
 * one instanced mesh per kind so the whole sky of them is a single draw. Rain streaks down and dies on the ground,
 * snow and ash drift down and rest a while where they land, sand blows low and fast across the ground, leaves tumble
 * on the wind. Ultra Dynamic Weather keeps the sky, fog, wind and the snow / wet / dust state of the surfaces; its own
 * sprite particles are switched off by Tools/playtest/weather.py so only these show. Intensity 0..1 scales the spawn
 * rate (0 = off); Wind is cm/s in world space.
 */
UCLASS()
class VOXELRPG_API AWotVoxelWeather : public AActor
{
	GENERATED_BODY()

public:
	AWotVoxelWeather();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather")
	TObjectPtr<UInstancedStaticMeshComponent> Cubes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWotWeatherKind Kind = EWotWeatherKind::Snow;

	/** 0 = nothing falls, 1 = the full SpawnRate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = 0.0, ClampMax = 2.0))
	float Intensity = 1.0f;

	/** Particles per second at Intensity 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SpawnRate = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 MaxParticles = 1500;

	/** Half-size of the box around the player the particles live in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FVector Area = FVector(2600.0f, 2600.0f, 900.0f);

	/** Wind, cm/s, world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FVector Wind = FVector(120.0f, 40.0f, 0.0f);

	/** The cube asset and the palette material (set per kind by ApplyKindDefaults when left empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	TObjectPtr<UMaterialInterface> Material;

	/** Per-kind look, filled by ApplyKindDefaults; editable afterwards. Size in cm, Scale per axis, FallSpeed cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	float Size = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	float FallSpeed = 300.0f;

	/** Sideways wander (cm/s) from per-particle noise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	float Drift = 60.0f;

	/** Seconds a landed particle rests before it shrinks away (0 = dies on landing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	float SettleSeconds = 0.0f;

	/** Leaves and ash tumble. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	float TumbleDegPerSec = 0.0f;

	/** Rain leaves a flat splash cube for a moment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Look")
	bool bSplash = false;

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void ApplyKindDefaults();

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void Configure(EWotWeatherKind InKind, float InIntensity);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	struct FParticle
	{
		FVector Pos = FVector::ZeroVector;
		FVector Vel = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		FVector Spin = FVector::ZeroVector;
		float GroundZ = -1e9f;
		float Age = 0.0f;
		float Landed = -1.0f;   // seconds since landing, < 0 while in the air
		float Phase = 0.0f;
		float SizeMul = 1.0f;
		bool bSplash = false;
	};
	TArray<FParticle> Particles;
	TArray<FTransform> Transforms;
	float SpawnAccumulator = 0.0f;
	double Time = 0.0;

	FVector Centre() const;
	void Spawn(const FVector& Centre, int32 Count);
	float GroundAt(const FVector& P) const;
	void WriteInstances();
};
