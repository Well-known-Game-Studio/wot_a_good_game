#include "Ways/WotMachinShin.h"
#include "WotGameplayFunctionLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AWotMachinShin::AWotMachinShin()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Shards = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Shards"));
	Shards->SetupAttachment(Root);
	Shards->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shards->SetCastShadow(false);
	Shards->SetCanEverAffectNavigation(false);
	Reach = CreateDefaultSubobject<USphereComponent>(TEXT("Reach"));
	Reach->SetupAttachment(Root);
	Reach->SetSphereRadius(360.0f);
	Reach->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Reach->SetCollisionResponseToAllChannels(ECR_Ignore);
	Reach->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ShardMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
	ShardMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Wot/Towns/Common/Materials/P_Black.P_Black")));
}

void AWotMachinShin::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildShards();
}

void AWotMachinShin::RebuildShards()
{
	if (!Shards) {
		return;
	}
	if (UStaticMesh* Mesh = ShardMesh.LoadSynchronous()) {
		Shards->SetStaticMesh(Mesh);
	}
	if (UMaterialInterface* Mat = ShardMaterial.LoadSynchronous()) {
		Shards->SetMaterial(0, Mat);
	}
	Shards->ClearInstances();
	ShardOffsets.Reset();
	ShardPhase.Reset();
	FRandomStream Rand(GetUniqueID());
	for (int32 i = 0; i < ShardCount; ++i) {
		// a flattened cloud, denser toward the middle
		const FVector Dir = Rand.GetUnitVector();
		const float R = SwarmRadius * FMath::Pow(Rand.FRand(), 0.6f);
		FVector Off = Dir * R;
		Off.Z *= 0.55f;
		ShardOffsets.Add(Off);
		ShardPhase.Add(Rand.FRand() * 1000.0f);
		const float S = ShardSize * Rand.FRandRange(0.4f, 1.3f) / 100.0f;
		Shards->AddInstance(FTransform(FRotator(Rand.FRand() * 360.0f, Rand.FRand() * 360.0f, 0), Off, FVector(S)), false);
	}
}

void AWotMachinShin::BeginPlay()
{
	Super::BeginPlay();
	if (Shards->GetInstanceCount() == 0) {
		RebuildShards();
	}
	Target = PatrolPoints.Num() > 0 ? PatrolPoints[0] : GetActorLocation();
}

void AWotMachinShin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	const FVector Loc = GetActorLocation();

	// --- listening / hunting
	if (Player) {
		const float Dist = FVector::Dist(Loc, Player->GetActorLocation());
		if (!bHunting && Dist < HearingRange) {
			bHunting = true;
		} else if (bHunting && Dist > LoseRange) {
			bHunting = false;
		}
	}
	FVector Goal = Target;
	float Speed = WanderSpeed;
	if (bHunting && Player) {
		Goal = Player->GetActorLocation() + FVector(0, 0, 120.0f);
		Speed = HuntSpeed;
	} else if (PatrolPoints.Num() > 0) {
		if (FVector::Dist2D(Loc, Target) < 250.0f) {
			PatrolIndex = (PatrolIndex + 1 + (FMath::Rand() % FMath::Max(1, PatrolPoints.Num() - 1))) % PatrolPoints.Num();
			Target = PatrolPoints[PatrolIndex] + FVector(0, 0, 160.0f);
		}
		Goal = Target;
	}
	FVector Desired = (Goal - Loc);
	if (!Desired.IsNearlyZero()) {
		Desired = Desired.GetSafeNormal() * Speed;
	}
	Velocity = FMath::VInterpTo(Velocity, Desired, DeltaTime, 1.6f);
	SetActorLocation(Loc + Velocity * DeltaTime);

	// --- the murmuration: shards orbit and stream around the centre
	const int32 N = FMath::Min(ShardOffsets.Num(), Shards->GetInstanceCount());
	const FVector Flow = Velocity.GetSafeNormal();
	for (int32 i = 0; i < N; ++i) {
		const float Ph = ShardPhase[i];
		const FVector Base = ShardOffsets[i];
		const float Swirl = Time * (0.6f + 0.4f * FMath::Sin(Ph));
		const FVector Rot = Base.RotateAngleAxis(FMath::RadiansToDegrees(Swirl), FVector::UpVector);
		const FVector Wobble(FMath::Sin(Time * 2.1f + Ph) * 40.0f, FMath::Cos(Time * 1.7f + Ph * 0.7f) * 40.0f, FMath::Sin(Time * 2.9f + Ph * 1.3f) * 30.0f);
		const FVector Trail = -Flow * (120.0f * FMath::Frac(Ph / 7.0f));
		const FVector P = Rot + Wobble + Trail;
		const FRotator R(Time * 90.0f + Ph, Time * 60.0f + Ph * 2.0f, 0);
		FTransform T;
		Shards->GetInstanceTransform(i, T, false);
		T.SetLocation(P);
		T.SetRotation(R.Quaternion());
		Shards->UpdateInstanceTransform(i, T, false, i == N - 1, false);
	}

	// --- touch: drain and throw back
	if (Player && Reach && Reach->IsOverlappingActor(Player)) {
		DamageAccumulator += DamagePerSecond * DeltaTime;
		if (DamageAccumulator >= 4.0f) {
			FHitResult Hit;
			Hit.ImpactPoint = Player->GetActorLocation();
			Hit.ImpactNormal = (Player->GetActorLocation() - Loc).GetSafeNormal();
			Hit.Location = Hit.ImpactPoint;
			Hit.TraceStart = Loc;
			Hit.TraceEnd = Player->GetActorLocation();
			UWotGameplayFunctionLibrary::ApplyDirectionalDamage(this, Player, -DamageAccumulator, Hit, Knockback);
			DamageAccumulator = 0.0f;
		}
	}
}
