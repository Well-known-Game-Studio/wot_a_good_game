#include "Weather/WotVoxelWeather.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "CollisionQueryParams.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"

static const TCHAR* GWotPalette = TEXT("/Game/Wot/Towns/Common/Materials");

AWotVoxelWeather::AWotVoxelWeather()
{
	PrimaryActorTick.bCanEverTick = true;
	Cubes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cubes"));
	SetRootComponent(Cubes);
	Cubes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cubes->SetCastShadow(false);
	Cubes->SetCanEverAffectNavigation(false);
	Cubes->SetMobility(EComponentMobility::Movable);
	Cubes->NumCustomDataFloats = 0;
	CubeMesh = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad());
}

void AWotVoxelWeather::ApplyKindDefaults()
{
	auto Pal = [](const TCHAR* Name) { return Cast<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), GWotPalette, Name, Name)).TryLoad()); };
	switch (Kind) {
	case EWotWeatherKind::Rain:
		Material = Pal(TEXT("P_Blue")); Size = 7.0f; Scale = FVector(0.5f, 0.5f, 9.0f); FallSpeed = 1500.0f; Drift = 10.0f; SettleSeconds = 0.0f; TumbleDegPerSec = 0.0f; bSplash = true;
		SpawnRate = 1100.0f; MaxParticles = 1600; Area = FVector(2400.0f, 2400.0f, 900.0f); break;
	case EWotWeatherKind::Snow:
		Material = Pal(TEXT("P_White")); Size = 16.0f; Scale = FVector(1.0f, 1.0f, 0.8f); FallSpeed = 150.0f; Drift = 70.0f; SettleSeconds = 7.0f; TumbleDegPerSec = 40.0f; bSplash = false;
		SpawnRate = 320.0f; MaxParticles = 1800; Area = FVector(2600.0f, 2600.0f, 900.0f); break;
	case EWotWeatherKind::Ash:
		Material = Pal(TEXT("P_DarkGrey")); Size = 13.0f; Scale = FVector(1.0f, 1.0f, 0.4f); FallSpeed = 70.0f; Drift = 110.0f; SettleSeconds = 5.0f; TumbleDegPerSec = 90.0f; bSplash = false;
		SpawnRate = 220.0f; MaxParticles = 1500; Area = FVector(2600.0f, 2600.0f, 1000.0f); break;
	case EWotWeatherKind::Sand:
		Material = Pal(TEXT("P_Clay")); Size = 12.0f; Scale = FVector(3.0f, 1.0f, 0.5f); FallSpeed = 40.0f; Drift = 90.0f; SettleSeconds = 0.0f; TumbleDegPerSec = 0.0f; bSplash = false;
		SpawnRate = 500.0f; MaxParticles = 1600; Area = FVector(2600.0f, 2600.0f, 260.0f); break;
	case EWotWeatherKind::Leaves:
		Material = Pal(TEXT("P_Ochre")); Size = 18.0f; Scale = FVector(1.0f, 1.3f, 0.25f); FallSpeed = 90.0f; Drift = 140.0f; SettleSeconds = 9.0f; TumbleDegPerSec = 220.0f; bSplash = false;
		SpawnRate = 30.0f; MaxParticles = 400; Area = FVector(2600.0f, 2600.0f, 700.0f); break;
	}
}

void AWotVoxelWeather::Configure(EWotWeatherKind InKind, float InIntensity)
{
	Kind = InKind; Intensity = InIntensity; Material = nullptr;
	ApplyKindDefaults();
	if (Cubes) {
		Cubes->ClearInstances();
		Cubes->SetStaticMesh(CubeMesh);
		if (Material) { Cubes->SetMaterial(0, Material); }
	}
	Particles.Reset();
}

void AWotVoxelWeather::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Material) { ApplyKindDefaults(); }
	if (Cubes) {
		Cubes->SetStaticMesh(CubeMesh);
		if (Material) { Cubes->SetMaterial(0, Material); }
	}
}

void AWotVoxelWeather::BeginPlay()
{
	Super::BeginPlay();
	if (!Material) { ApplyKindDefaults(); }
	if (Cubes) {
		Cubes->SetStaticMesh(CubeMesh);
		if (Material) { Cubes->SetMaterial(0, Material); }
		Cubes->ClearInstances();
	}
	Particles.Reset();
	// start with the air already full
	const FVector C = Centre();
	const int32 Prefill = FMath::Min(MaxParticles, (int32)(SpawnRate * Intensity * 2.0f));
	Spawn(C, Prefill);
	for (FParticle& P : Particles) { P.Pos.Z = FMath::FRandRange(P.GroundZ + 10.0f, C.Z + Area.Z); }
}

FVector AWotVoxelWeather::Centre() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Pawn ? Pawn->GetActorLocation() : GetActorLocation();
}

float AWotVoxelWeather::GroundAt(const FVector& P) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotVoxelWeather), false);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 100.0f), P - FVector(0, 0, 4000.0f), ECC_Visibility, Params)) {
		return Hit.ImpactPoint.Z;
	}
	return P.Z - 4000.0f;
}

void AWotVoxelWeather::Spawn(const FVector& C, int32 Count)
{
	for (int32 i = 0; i < Count && Particles.Num() < MaxParticles; ++i) {
		FParticle P;
		P.Pos = FVector(C.X + FMath::FRandRange(-Area.X, Area.X), C.Y + FMath::FRandRange(-Area.Y, Area.Y), 0.0f);
		P.GroundZ = GroundAt(FVector(P.Pos.X, P.Pos.Y, C.Z + Area.Z));
		if (Kind == EWotWeatherKind::Sand) {
			// sand runs low over the ground, into the wind
			P.Pos.Z = P.GroundZ + FMath::FRandRange(8.0f, Area.Z);
			P.Vel = Wind * FMath::FRandRange(2.0f, 3.5f) + FVector(0, 0, -FallSpeed);
			P.Rot = Wind.IsNearlyZero() ? FRotator::ZeroRotator : Wind.Rotation();
		} else {
			P.Pos.Z = C.Z + Area.Z + FMath::FRandRange(0.0f, 200.0f);
			P.Vel = FVector(0, 0, -FallSpeed * FMath::FRandRange(0.8f, 1.25f));
			P.Rot = FRotator(FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f));
		}
		P.Spin = TumbleDegPerSec > 0.0f ? FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f)).GetSafeNormal() * TumbleDegPerSec : FVector::ZeroVector;
		P.Phase = FMath::FRandRange(0.0f, 6.28f);
		P.SizeMul = FMath::FRandRange(0.7f, 1.3f);
		Particles.Add(P);
	}
}

void AWotVoxelWeather::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Cubes || !CubeMesh) { return; }
	Time += DeltaTime;
	const FVector C = Centre();
	// spawn
	SpawnAccumulator += SpawnRate * Intensity * DeltaTime;
	const int32 N = (int32)SpawnAccumulator; SpawnAccumulator -= N;
	if (N > 0) { Spawn(C, N); }
	// move
	for (int32 i = Particles.Num() - 1; i >= 0; --i) {
		FParticle& P = Particles[i];
		P.Age += DeltaTime;
		if (P.Landed >= 0.0f) {
			P.Landed += DeltaTime;
			const float Fade = SettleSeconds > 0.0f ? 1.0f - FMath::Clamp((P.Landed - SettleSeconds) / 1.5f, 0.0f, 1.0f) : 0.0f;
			if (Fade <= 0.0f && P.Landed > (P.bSplash ? 0.18f : SettleSeconds)) { Particles.RemoveAtSwap(i); continue; }
			continue;
		}
		const float N1 = FMath::PerlinNoise1D(Time * 0.9 + P.Phase), N2 = FMath::PerlinNoise1D(Time * 0.7 + P.Phase * 1.7 + 40.0);
		FVector V = P.Vel + Wind + FVector(N1, N2, 0.0f) * Drift;
		if (Kind == EWotWeatherKind::Leaves || Kind == EWotWeatherKind::Ash) { V.Z += FMath::PerlinNoise1D(Time * 1.3 + P.Phase * 2.3) * Drift * 0.6f; }
		P.Pos += V * DeltaTime;
		if (!P.Spin.IsNearlyZero()) { P.Rot += FRotator(P.Spin.Y, P.Spin.Z, P.Spin.X) * DeltaTime; }
		// out of the box around the player: recycle
		if (FMath::Abs(P.Pos.X - C.X) > Area.X * 1.2f || FMath::Abs(P.Pos.Y - C.Y) > Area.Y * 1.2f || P.Pos.Z < C.Z - 2500.0f) { Particles.RemoveAtSwap(i); continue; }
		if (Kind == EWotWeatherKind::Sand) {
			if (P.Pos.Z < P.GroundZ + 6.0f) { P.Pos.Z = P.GroundZ + 6.0f; P.Vel.Z = 0.0f; }
			continue;
		}
		if (P.Pos.Z <= P.GroundZ + Size * 0.5f) {
			P.Pos.Z = P.GroundZ + Size * 0.5f * (bSplash ? 0.3f : Scale.Z);
			if (bSplash) { P.bSplash = true; P.Landed = 0.0f; P.Rot = FRotator::ZeroRotator; }
			else if (SettleSeconds > 0.0f) { P.Landed = 0.0f; P.Rot = FRotator(0, P.Rot.Yaw, 0); }
			else { Particles.RemoveAtSwap(i); continue; }
		}
	}
	WriteInstances();
}

void AWotVoxelWeather::WriteInstances()
{
	Transforms.SetNum(Particles.Num(), EAllowShrinking::No);
	for (int32 i = 0; i < Particles.Num(); ++i) {
		const FParticle& P = Particles[i];
		float S = Size * P.SizeMul / 100.0f;   // the cube asset is 100 cm
		FVector Sc = Scale;
		if (P.bSplash) { Sc = FVector(4.0f, 4.0f, 0.3f); S *= 1.0f - FMath::Clamp(P.Landed / 0.18f, 0.0f, 0.9f); }
		else if (P.Landed > SettleSeconds && SettleSeconds > 0.0f) { S *= FMath::Max(0.0f, 1.0f - (P.Landed - SettleSeconds) / 1.5f); }
		Transforms[i] = FTransform(P.Rot, P.Pos, Sc * S);
	}
	const int32 Have = Cubes->GetInstanceCount();
	if (Have < Transforms.Num()) {
		TArray<FTransform> Extra(Transforms.GetData() + Have, Transforms.Num() - Have);
		Cubes->AddInstances(Extra, false, true);
	} else if (Have > Transforms.Num()) {
		for (int32 i = Have - 1; i >= Transforms.Num(); --i) { Cubes->RemoveInstance(i); }
	}
	if (Transforms.Num() > 0) { Cubes->BatchUpdateInstancesTransforms(0, Transforms, true, true, false); }
}

// wot.Weather.Voxel <Rain|Snow|Ash|Sand|Leaves|Off> [Intensity] : set (or spawn) the voxel weather in the play world
static FAutoConsoleCommandWithWorldAndArgs GWotWeatherVoxel(
	TEXT("wot.Weather.Voxel"), TEXT("Voxel weather in this world: wot.Weather.Voxel Rain|Snow|Ash|Sand|Leaves|Off [Intensity]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (!World || Args.Num() < 1) { return; }
		AWotVoxelWeather* W = nullptr;
		TActorIterator<AWotVoxelWeather> It(World);
		if (It) { W = *It; }
		const FString K = Args[0].ToLower();
		if (K == TEXT("off")) { if (W) { W->Intensity = 0.0f; } return; }
		if (!W) { W = World->SpawnActor<AWotVoxelWeather>(); }
		if (!W) { return; }
		const EWotWeatherKind Kind = K == TEXT("rain") ? EWotWeatherKind::Rain : K == TEXT("ash") ? EWotWeatherKind::Ash : K == TEXT("sand") ? EWotWeatherKind::Sand : K == TEXT("leaves") ? EWotWeatherKind::Leaves : EWotWeatherKind::Snow;
		W->Configure(Kind, Args.Num() > 1 ? FCString::Atof(*Args[1]) : 1.0f);
		UE_LOG(LogTemp, Display, TEXT("[PT] WEATHER voxel %s intensity %.2f"), *Args[0], W->Intensity);
	}));
