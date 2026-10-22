#include "Weather/WotVoxelWater.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"

namespace
{
	constexpr float kBucket = 200.0f;
	FIntPoint Bucket(const FVector2D& P) { return FIntPoint(FMath::FloorToInt(P.X / kBucket), FMath::FloorToInt(P.Y / kBucket)); }
}

AWotVoxelWater::AWotVoxelWater()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Flecks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Flecks"));
	Flecks->SetupAttachment(RootComponent);
	Flecks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flecks->SetCastShadow(false);
	Flecks->SetMobility(EComponentMobility::Movable);
	Flecks->NumCustomDataFloats = 0;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) { CubeMesh = Cube.Object; }
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Pale(TEXT("/Game/Wot/Towns/Common/Materials/P_White.P_White"));
	if (Pale.Succeeded()) { Material = Pale.Object; }
}

void AWotVoxelWater::GatherCells()
{
	Cells.Reset(); Grid.Reset(); FallCells.Reset();
	UWorld* World = GetWorld();
	// every static mesh component wearing the voxel water material: PCG instanced tiles (rivers, canals), and plain
	// static meshes (plaza pools, the waterfall sheets) as single instances
	for (TObjectIterator<UStaticMeshComponent> It; It; ++It) {
		UStaticMeshComponent* SMC = *It;
		if (SMC->GetWorld() != World || !SMC->IsRegistered() || SMC == Flecks) { continue; }
		UMaterialInterface* M = SMC->GetMaterial(0);
		if (!M || !M->GetName().Contains(TEXT("VoxelWater"))) { continue; }
		UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(SMC);
		const FBox MeshBox = SMC->GetStaticMesh() ? SMC->GetStaticMesh()->GetBoundingBox() : FBox(FVector(-50), FVector(50));
		const int32 Count = ISM ? ISM->GetInstanceCount() : 1;
		for (int32 i = 0; i < Count; ++i) {
			FTransform T = SMC->GetComponentTransform();
			if (ISM && !ISM->GetInstanceTransform(i, T, true)) { continue; }
			if (!ISM && MeshBox.GetExtent().Z * T.GetScale3D().Z > MeshBox.GetExtent().X * T.GetScale3D().X * 2) { continue; }   // a waterfall sheet, not a surface
			const FVector S = T.GetScale3D(); const FVector C = T.GetLocation();
			FCell Cell; Cell.Centre = FVector2D(C.X, C.Y); Cell.HalfX = MeshBox.GetExtent().X * S.X; Cell.HalfY = MeshBox.GetExtent().Y * S.Y;
			Cell.Top = C.Z + (MeshBox.Max.Z) * S.Z; Cell.Axis = FVector2D(1, 0); Cell.FallTo = -1;
			// big water tiles (plaza pools, canals) are split into ~200 cm cells so streaks and ripples are evenly spread
			const int32 NX = FMath::Max(1, FMath::RoundToInt(Cell.HalfX * 2 / 200.0f)), NY = FMath::Max(1, FMath::RoundToInt(Cell.HalfY * 2 / 200.0f));
			for (int32 ix = 0; ix < NX; ++ix) for (int32 iy = 0; iy < NY; ++iy) {
				FCell Sub = Cell; Sub.HalfX = Cell.HalfX / NX; Sub.HalfY = Cell.HalfY / NY;
				Sub.Centre = Cell.Centre + FVector2D(-Cell.HalfX + (ix + 0.5f) * 2 * Sub.HalfX, -Cell.HalfY + (iy + 0.5f) * 2 * Sub.HalfY);
				Cells.Add(Sub);
			}
		}
	}
	for (int32 i = 0; i < Cells.Num(); ++i) { Grid.FindOrAdd(Bucket(Cells[i].Centre)).Add(i); }
	// the local run of the water: principal axis of the neighbouring cell centres within 500 cm, pointed downstream
	const FVector2D Down = FVector2D(Flow.X, Flow.Y).GetSafeNormal();
	for (int32 i = 0; i < Cells.Num(); ++i) {
		FCell& Cell = Cells[i]; double Sxx = 0, Sxy = 0, Syy = 0; int32 N = 0; float LowestTop = Cell.Top; int32 Lowest = -1;
		const FIntPoint B = Bucket(Cell.Centre);
		for (int32 dx = -3; dx <= 3; ++dx) for (int32 dy = -3; dy <= 3; ++dy) {
			if (const TArray<int32>* L = Grid.Find(B + FIntPoint(dx, dy))) {
				for (int32 j : *L) {
					const FVector2D D = Cells[j].Centre - Cell.Centre; const float Dist = D.Size();
					if (Dist > 500.0f) { continue; }
					if (FMath::Abs(Cells[j].Top - Cell.Top) < 60.0f) { Sxx += D.X * D.X; Sxy += D.X * D.Y; Syy += D.Y * D.Y; ++N; }
					else if (Dist < 260.0f && Cells[j].Top < LowestTop - 150.0f) { LowestTop = Cells[j].Top; Lowest = j; }
				}
			}
		}
		if (N > 2) {
			const double Th = 0.5 * FMath::Atan2(2 * Sxy, Sxx - Syy);
			Cell.Axis = FVector2D(FMath::Cos(Th), FMath::Sin(Th));
			if (FVector2D::DotProduct(Cell.Axis, Down) < 0) { Cell.Axis *= -1; }
		}
		else { Cell.Axis = Down.IsNearlyZero() ? FVector2D(1, 0) : Down; }
		Cell.FallTo = Lowest;
		if (Lowest >= 0) { FallCells.Add(i); }
	}
	UE_LOG(LogTemp, Display, TEXT("[WATER] %s: %d water cells, %d fall edges, flow (%.2f %.2f)"), *GetName(), Cells.Num(), FallCells.Num(), Down.X, Down.Y);
}

int32 AWotVoxelWater::CellAt(const FVector& P) const
{
	const FIntPoint B = Bucket(FVector2D(P.X, P.Y));
	for (int32 dx = -1; dx <= 1; ++dx) for (int32 dy = -1; dy <= 1; ++dy) {
		if (const TArray<int32>* L = Grid.Find(B + FIntPoint(dx, dy))) {
			for (int32 j : *L) {
				const FCell& C = Cells[j];
				if (FMath::Abs(P.X - C.Centre.X) <= C.HalfX && FMath::Abs(P.Y - C.Centre.Y) <= C.HalfY) { return j; }
			}
		}
	}
	return -1;
}

void AWotVoxelWater::BeginPlay()
{
	Super::BeginPlay();
	if (CubeMesh) { Flecks->SetStaticMesh(CubeMesh); }
	if (Material) { Flecks->SetMaterial(0, Material); }
	Flecks->ClearInstances();
	GatherCells();
	SetActorTickEnabled(bEnabled && Cells.Num() > 0);
}

void AWotVoxelWater::SpawnStreak()
{
	if (Cells.Num() == 0) { return; }
	const FCell& C = Cells[FMath::RandRange(0, Cells.Num() - 1)];
	FFleck F; F.Kind = 0;
	F.Pos = FVector(C.Centre.X + FMath::FRandRange(-C.HalfX, C.HalfX), C.Centre.Y + FMath::FRandRange(-C.HalfY, C.HalfY), C.Top + 3.0f);
	const FVector2D Side(-C.Axis.Y, C.Axis.X);
	const FVector2D V = C.Axis * StreakSpeed * FMath::FRandRange(0.7f, 1.3f) + Side * FMath::FRandRange(-6.0f, 6.0f);
	F.Vel = FVector(V.X, V.Y, 0); F.Age = 0; F.Life = FMath::FRandRange(2.2f, 4.0f);
	F.Size = StreakSize * FMath::FRandRange(0.6f, 1.3f); F.Ring = FVector2D::ZeroVector; F.RingSpeed = 0;
	Live.Add(F);
}

void AWotVoxelWater::SpawnFall()
{
	if (FallCells.Num() == 0) { return; }
	const FCell& C = Cells[FallCells[FMath::RandRange(0, FallCells.Num() - 1)]];
	const FCell& Low = Cells[C.FallTo];
	const FVector2D Dir = (Low.Centre - C.Centre).GetSafeNormal(); const FVector2D Side(-Dir.Y, Dir.X);
	const FVector2D Lip = C.Centre + Dir * FMath::Max(C.HalfX, C.HalfY) + Side * FMath::FRandRange(-C.HalfY, C.HalfY);
	FFleck F; F.Kind = 2;
	F.Pos = FVector(Lip.X, Lip.Y, C.Top + 6.0f);
	F.Vel = FVector(Dir.X, Dir.Y, 0) * 80.0f + FVector(0, 0, -60.0f);
	F.Age = 0; F.Life = FMath::Sqrt(2.0f * FMath::Max(100.0f, C.Top - Low.Top) / 980.0f) + 0.15f;   // time to fall the drop under gravity
	F.Size = FVector(14, 14, 24); F.Ring = FVector2D::ZeroVector; F.RingSpeed = 0;
	Live.Add(F);
}

void AWotVoxelWater::SpawnRipple(const FVector& Centre, float Strength)
{
	const int32 Cell = CellAt(Centre);
	if (Cell < 0) { return; }
	const float Top = Cells[Cell].Top;
	const int32 N = 16;
	for (int32 k = 0; k < N; ++k) {
		const float A = 2 * PI * k / N + FMath::FRandRange(-0.1f, 0.1f);
		FFleck F; F.Kind = 1;
		F.Ring = FVector2D(FMath::Cos(A), FMath::Sin(A)); F.RingSpeed = RippleRadius / RippleSeconds * FMath::FRandRange(0.85f, 1.15f) * Strength;
		F.Pos = FVector(Centre.X + F.Ring.X * 22.0f, Centre.Y + F.Ring.Y * 22.0f, Top + 3.0f);
		F.Vel = FVector::ZeroVector; F.Age = 0; F.Life = RippleSeconds;
		F.Size = FVector(26, 13, 6) * FMath::Clamp(Strength, 0.6f, 1.4f);
		Live.Add(F);
	}
	// a few drops kicked up by a moving wader
	if (Strength > 1.0f) {
		for (int32 k = 0; k < 4; ++k) {
			FFleck F; F.Kind = 2; F.Pos = Centre + FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), 0); F.Pos.Z = Top + 4;
			F.Vel = FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(120.f, 220.f));
			F.Age = 0; F.Life = 0.5f; F.Size = FVector(8, 8, 8); F.Ring = FVector2D::ZeroVector; F.RingSpeed = 0;
			Live.Add(F);
		}
	}
}

void AWotVoxelWater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bEnabled || Cells.Num() == 0) { return; }
	// streaks: keep StreakDensity of them per cell alive; falls: a steady trickle per fall edge
	const int32 WantStreaks = FMath::Min(MaxFlecks * 2 / 3, FMath::RoundToInt(Cells.Num() * StreakDensity));
	int32 NumStreaks = 0; for (const FFleck& F : Live) { if (F.Kind == 0) { ++NumStreaks; } }
	StreakAccumulator += DeltaTime * FMath::Max(0, WantStreaks - NumStreaks) * 1.5f;
	while (StreakAccumulator >= 1.0f && Live.Num() < MaxFlecks) { SpawnStreak(); StreakAccumulator -= 1.0f; }
	FallAccumulator += DeltaTime * FallCells.Num() * 6.0f;
	while (FallAccumulator >= 1.0f && Live.Num() < MaxFlecks) { SpawnFall(); FallAccumulator -= 1.0f; }
	// waders: anyone whose feet are below a water cell's surface
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It) {
		ACharacter* C = *It; if (!C) { continue; }
		const float Half = C->GetCapsuleComponent() ? C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.0f;
		const FVector Feet = C->GetActorLocation() - FVector(0, 0, Half - 4.0f);
		const int32 Cell = CellAt(Feet);
		FWader* W = Waders.FindByPredicate([C](const FWader& X) { return X.Actor.Get() == C; });
		if (Cell < 0 || Feet.Z > Cells[Cell].Top + 30.0f) { if (W) { W->Timer = 0; } continue; }
		if (!W) { W = &Waders.AddDefaulted_GetRef(); W->Actor = C; W->Timer = 0; }
		const float Speed = C->GetVelocity().Size2D();
		W->Timer -= DeltaTime;
		if (W->Timer <= 0.0f) {
			const bool bMoving = Speed > 15.0f;
			SpawnRipple(FVector(Feet.X, Feet.Y, Cells[Cell].Top), bMoving ? 1.3f : 0.8f);
			W->Timer = bMoving ? 0.22f : 0.55f;
		}
	}
	Waders.RemoveAll([](const FWader& X) { return !X.Actor.IsValid(); });
	// move and age
	for (int32 i = Live.Num() - 1; i >= 0; --i) {
		FFleck& F = Live[i]; F.Age += DeltaTime;
		if (F.Age >= F.Life) { Live.RemoveAtSwap(i); continue; }
		if (F.Kind == 1) { F.Pos.X += F.Ring.X * F.RingSpeed * DeltaTime; F.Pos.Y += F.Ring.Y * F.RingSpeed * DeltaTime; }
		else if (F.Kind == 2) { F.Vel.Z -= 980.0f * DeltaTime; F.Pos += F.Vel * DeltaTime; }
		else { F.Pos += F.Vel * DeltaTime; }
	}
	WriteInstances();
}

FString AWotVoxelWater::Describe() const
{
	int32 K[3] = {0, 0, 0}; for (const FFleck& F : Live) { if (F.Kind < 3) { ++K[F.Kind]; } }
	FString W;
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It) {
		ACharacter* C = *It; const float Half = C->GetCapsuleComponent() ? C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.0f;
		const FVector Feet = C->GetActorLocation() - FVector(0, 0, Half - 4.0f); const int32 Cell = CellAt(Feet);
		if (C->IsPlayerControlled() || Cell >= 0) { W += FString::Printf(TEXT(" %s feet=(%.0f %.0f %.0f) cell=%d top=%.0f;"), *C->GetName(), Feet.X, Feet.Y, Feet.Z, Cell, Cell >= 0 ? Cells[Cell].Top : 0.f); }
	}
	return FString::Printf(TEXT("live streaks=%d rings=%d drops=%d waders=%d |%s"), K[0], K[1], K[2], Waders.Num(), *W);
}

void AWotVoxelWater::WriteInstances()
{
	TArray<FTransform> Transforms; Transforms.Reserve(Live.Num());
	for (const FFleck& F : Live) {
		const float T = F.Age / F.Life;
		float Fade = 1.0f;
		if (F.Kind == 0) { Fade = FMath::Min(1.0f, FMath::Min(T * 4.0f, (1.0f - T) * 4.0f)); }           // in and out
		else if (F.Kind == 1) { Fade = 1.0f - T * T; }                                                      // rings thin as they spread
		FVector Size = F.Size * FMath::Max(0.05f, Fade) / 100.0f;
		FRotator Rot = FRotator::ZeroRotator;
		if (F.Kind == 0) { Rot.Yaw = FMath::RadiansToDegrees(FMath::Atan2(F.Vel.Y, F.Vel.X)); }
		else if (F.Kind == 1) { Rot.Yaw = FMath::RadiansToDegrees(FMath::Atan2(F.Ring.Y, F.Ring.X)) + 90.0f; }
		Transforms.Add(FTransform(Rot, F.Pos, Size));
	}
	const int32 Have = Flecks->GetInstanceCount();
	if (Transforms.Num() > Have) {
		TArray<FTransform> Extra; for (int32 i = Have; i < Transforms.Num(); ++i) { Extra.Add(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.001f))); }
		Flecks->AddInstances(Extra, false, true);
	}
	for (int32 i = Transforms.Num(); i < Flecks->GetInstanceCount(); ++i) { Transforms.Add(FTransform(FRotator::ZeroRotator, FVector(0, 0, -100000), FVector(0.001f))); }
	if (Transforms.Num() > 0) { Flecks->BatchUpdateInstancesTransforms(0, Transforms, true, true, false); }
}

// wot.Water.Status : how much water the actor found in this world
static FAutoConsoleCommandWithWorld GWotWaterStatus(
	TEXT("wot.Water.Status"), TEXT("Log the voxel water actor's cells and fall edges"),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World) {
		int32 N = 0;
		for (TActorIterator<AWotVoxelWater> It(World); It; ++It) { ++N; UE_LOG(LogTemp, Display, TEXT("[WATER] %s cells=%d enabled=%d %s"), *It->GetName(), It->NumCells(), It->bEnabled ? 1 : 0, *It->Describe()); }
		if (N == 0) { UE_LOG(LogTemp, Display, TEXT("[WATER] no voxel water actor in this world")); }
	}));
