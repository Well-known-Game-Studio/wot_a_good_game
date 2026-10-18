#include "Channeling/WotChannelCameraShake.h"

void UWotChannelShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration::Infinite();
	OutInfo.BlendIn = 0.15f;
	OutInfo.BlendOut = 0.3f;
}

void UWotChannelShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Time = 0.0f;
	Seed = FMath::FRandRange(0.0f, 1000.0f);
	FadeWeight = 1.0f;
	bStopped = false;
}

void UWotChannelShakePattern::StopShakePatternImpl(const FCameraShakePatternStopParams& Params)
{
	bStopped = true;
	if (Params.bImmediately) {
		FadeWeight = 0.0f;
	}
}

void UWotChannelShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Time += Params.DeltaTime;
	if (bStopped) {
		FadeWeight = FMath::Max(0.0f, FadeWeight - Params.DeltaTime / 0.3f);
	}
	const float T = Time * Frequency + Seed;
	const float A = FMath::Clamp(Chaos, 0.0f, 2.0f) * Params.GetTotalScale() * FadeWeight;
	// three uncorrelated noise channels per axis group; PerlinNoise1D is in [-1, 1] and smooth
	const float P = FMath::PerlinNoise1D(T), Y = FMath::PerlinNoise1D(T + 37.1f), R = FMath::PerlinNoise1D(T * 0.7f + 91.3f);
	OutResult.Rotation = FRotator(P * RotationDegrees * A, Y * RotationDegrees * 0.8f * A, R * RotationDegrees * 0.5f * A);
	OutResult.Location = FVector(FMath::PerlinNoise1D(T * 1.3f + 11.0f), FMath::PerlinNoise1D(T * 1.3f + 53.0f), FMath::PerlinNoise1D(T * 1.1f + 77.0f)) * LocationCm * A;
	OutResult.FOV = FMath::PerlinNoise1D(T * 0.5f + 5.0f) * FOVDegrees * A;
	if (PostProcessWeight > 0.0f) {
		OutResult.PostProcessSettings = PostProcess;
		OutResult.PostProcessBlendWeight = FMath::Clamp(PostProcessWeight * FadeWeight, 0.0f, 1.0f);
	}
}

UWotChannelCameraShake::UWotChannelCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	Pattern = CreateDefaultSubobject<UWotChannelShakePattern>(TEXT("Pattern"));
	SetRootShakePattern(Pattern);
}
