#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "WotChannelCameraShake.generated.h"

/**
 * The shake of holding saidin. Perlin noise on the camera rotation (the view sits far from the character, so rotation
 * is what reads) plus a little FOV breathing. Runs until stopped; the channel component drives Chaos every frame:
 * clean saidin is a fast, fine tremor that surges, tainted saidin a slower, heavier lurch.
 */
UCLASS()
class VOXELRPG_API UWotChannelShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	/** 0 = still, 1 = full chaos; the component sets it every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float Chaos = 0.0f;

	/** Noise speed in Hz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float Frequency = 9.0f;

	/** Rotation amplitude in degrees at Chaos 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float RotationDegrees = 1.1f;

	/** Location amplitude in cm at Chaos 1 (small: the camera is far away). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float LocationCm = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float FOVDegrees = 0.5f;

	/** Post-process applied through the shake (camera shakes are the one place a per-frame blend reliably lands). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	FPostProcessSettings PostProcess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	float PostProcessWeight = 0.0f;

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override { return bStopped && FadeWeight <= 0.0f; }
	virtual void StopShakePatternImpl(const FCameraShakePatternStopParams& Params) override;

	float Time = 0.0f;
	float Seed = 0.0f;
	float FadeWeight = 1.0f;
	bool bStopped = false;
};

UCLASS()
class VOXELRPG_API UWotChannelCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UWotChannelCameraShake(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shake")
	TObjectPtr<UWotChannelShakePattern> Pattern;
};
