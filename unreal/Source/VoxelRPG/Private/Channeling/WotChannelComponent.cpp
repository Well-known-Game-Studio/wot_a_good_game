#include "Channeling/WotChannelComponent.h"
#include "Channeling/WotTiedWeave.h"
#include "Components/LocalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"
#include "Engine/StaticMesh.h"
#include "CollisionQueryParams.h"
#include "WotCharacter.h"
#include "WotActionComponent.h"
#include "WotAttributeComponent.h"
#include "WotGameplayFunctionLibrary.h"
#include "AI/WotAICharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Algo/Count.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotChannel, Log, All);

UWotChannelComponent::UWotChannelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	ThreadSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Wot/Effects/NS_WeaveThread.NS_WeaveThread")));
	TiedWeaveClass = AWotTiedWeave::StaticClass();
	// Ultimate Magic Bundle sets: Holy for the clean light of the Power, Shadow for the taint on saidin
	ThreadsSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Wot/Effects/Channeling_FX_NS.Channeling_FX_NS")));
	EmbraceSystemSaidar = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Holy/NS_Holy_Muzzle.NS_Holy_Muzzle")));
	EmbraceSystemSaidin = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Lightning/NS_Lightning_Muzzle.NS_Lightning_Muzzle")));
	TaintSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Shadow/NS_Shadow_Muzzle.NS_Shadow_Muzzle")));
	ReleaseSystems.Add(EWotWeave::Fire, TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Fire/NS_Fire_Muzzle.NS_Fire_Muzzle"))));
	ReleaseSystems.Add(EWotWeave::Air, TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Holy/NS_Holy_Impact.NS_Holy_Impact"))));
	ReleaseSystems.Add(EWotWeave::Water, TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Water/NS_Water_Impact.NS_Water_Impact"))));
	ReleaseSystems.Add(EWotWeave::Earth, TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Nature/NS_Nature_Impact.NS_Nature_Impact"))));
	FireProjectileClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Wot/Actors/Weapons/Projectiles/MagicProjectile.MagicProjectile_C")));
	EarthShieldTag = FGameplayTag::RequestGameplayTag(FName("Status.Parrying"), false);
}

AWotCharacter* UWotChannelComponent::GetCharacter() const
{
	return Cast<AWotCharacter>(GetOwner());
}

FText UWotChannelComponent::WeaveName(EWotWeave Weave)
{
	switch (Weave) {
	case EWotWeave::Fire: return NSLOCTEXT("Wot", "WeaveFire", "Fire");
	case EWotWeave::Air: return NSLOCTEXT("Wot", "WeaveAir", "Air");
	case EWotWeave::Water: return NSLOCTEXT("Wot", "WeaveWater", "Water");
	case EWotWeave::Earth: return NSLOCTEXT("Wot", "WeaveEarth", "Earth");
	}
	return FText::GetEmpty();
}

float UWotChannelComponent::GetCharge() const
{
	if (!bChanneling || !GetWorld()) {
		return 0.0f;
	}
	const double Held = GetWorld()->GetTimeSeconds() - ChannelStartTime;
	return FMath::Clamp(float(Held / FMath::Max(0.05f, FullChargeSeconds)), 0.0f, 1.0f);
}

UNiagaraComponent* UWotChannelComponent::AttachEffect(const TSoftObjectPtr<UNiagaraSystem>& SoftSystem, FName Socket, bool bLoop)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	UNiagaraSystem* System = SoftSystem.LoadSynchronous();
	if (!Mesh || !System) {
		return nullptr;
	}
	const FName UseSocket = Mesh->DoesSocketExist(Socket) ? Socket : NAME_None;
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Mesh, UseSocket, UseSocket.IsNone() ? FVector(0, 0, 120.0f) : FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget, !bLoop, true);
	if (Comp && bLoop) {
		ActiveEffects.Add(Comp);
	}
	return Comp;
}

void UWotChannelComponent::StopEffects()
{
	for (UNiagaraComponent* Comp : ActiveEffects) {
		if (Comp) {
			Comp->Deactivate();
			Comp->DestroyComponent();
		}
	}
	ActiveEffects.Reset();
}

void UWotChannelComponent::SpawnReleaseEffect(EWotWeave Weave, const FVector& Location)
{
	const TSoftObjectPtr<UNiagaraSystem>* Soft = ReleaseSystems.Find(Weave);
	UNiagaraSystem* System = Soft ? Soft->LoadSynchronous() : nullptr;
	if (System && GetWorld()) {
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Location, GetOwner()->GetActorRotation(), FVector(1.0f), true, true);
	}
}

void UWotChannelComponent::BeginChannel()
{
	AWotCharacter* Char = GetCharacter();
	if (!CanChannel() || bChanneling || !Char || !GetWorld()) {
		return;
	}
	bChanneling = true;
	ChannelStartTime = GetWorld()->GetTimeSeconds();
	if (ThreadSystem.IsNull()) {
		AttachEffect(ThreadsSystem, LeftHandSocket, true);
		AttachEffect(ThreadsSystem, RightHandSocket, true);
	} else {
		SpawnThreads();
		SetComponentTickEnabled(true);
	}
	AttachEffect(Half == EWotPowerHalf::Saidar ? EmbraceSystemSaidar : EmbraceSystemSaidin, RightHandSocket, false);
	if (Half == EWotPowerHalf::Saidin) {
		AttachEffect(TaintSystem, LeftHandSocket, true);
	}
	Char->ShowPopupWidget(FText::Format(NSLOCTEXT("Wot", "Channeling", "{0} ... {1}"), Half == EWotPowerHalf::Saidar ? NSLOCTEXT("Wot", "Embrace", "Embracing saidar") : NSLOCTEXT("Wot", "Seize", "Seizing saidin"), WeaveName(SelectedWeave)), 1.2f, false);
	UE_LOG(LogWotChannel, Log, TEXT("%s begins channeling (%s, weave %s)"), *GetNameSafe(Char), Half == EWotPowerHalf::Saidar ? TEXT("saidar") : TEXT("saidin"), *WeaveName(SelectedWeave).ToString());
}

void UWotChannelComponent::CancelChannel()
{
	if (!bChanneling) {
		return;
	}
	bChanneling = false;
	StopEffects();
	DestroyThreads();
	SetComponentTickEnabled(false);
}

void UWotChannelComponent::ReleaseWeave()
{
	if (!bChanneling) {
		return;
	}
	const float Charge = 0.35f + 0.65f * GetCharge();
	bChanneling = false;
	StopEffects();
	if (AActor* Owner = GetOwner()) {
		// the threads leap to the point the weave is loosed at, then fade
		const bool bOutward = SelectedWeave == EWotWeave::Fire || SelectedWeave == EWotWeave::Air;
		ReleaseThreads(bOutward ? Owner->GetActorLocation() + Owner->GetActorForwardVector() * 260.0f + FVector(0, 0, 70.0f) : Owner->GetActorLocation() + FVector(0, 0, 30.0f));
	}
	SetComponentTickEnabled(false);
	switch (SelectedWeave) {
	case EWotWeave::Fire: WeaveFire(Charge); break;
	case EWotWeave::Air: WeaveAir(Charge); break;
	case EWotWeave::Water: WeaveWater(Charge); break;
	case EWotWeave::Earth: WeaveEarth(Charge); break;
	}
	OnWeaveReleased.Broadcast(SelectedWeave, Charge);
	UE_LOG(LogWotChannel, Log, TEXT("%s releases %s at charge %.2f"), *GetNameSafe(GetOwner()), *WeaveName(SelectedWeave).ToString(), Charge);
}

void UWotChannelComponent::SelectWeave(EWotWeave Weave)
{
	SelectedWeave = Weave;
	if (AWotCharacter* Char = GetCharacter()) {
		Char->ShowPopupWidget(FText::Format(NSLOCTEXT("Wot", "WeaveSelected", "Weave: {0}"), WeaveName(Weave)), 1.0f, false);
	}
}

void UWotChannelComponent::NextWeave()
{
	SelectWeave(static_cast<EWotWeave>((static_cast<uint8>(SelectedWeave) + 1) % 4));
}

void UWotChannelComponent::PrevWeave()
{
	SelectWeave(static_cast<EWotWeave>((static_cast<uint8>(SelectedWeave) + 3) % 4));
}

void UWotChannelComponent::WeaveFire(float Charge)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UClass* ProjClass = FireProjectileClass.LoadSynchronous();
	if (!Char || !ProjClass || !GetWorld()) {
		return;
	}
	USkeletalMeshComponent* Mesh = Char->GetMesh();
	FVector Start = Mesh && Mesh->DoesSocketExist(RightHandSocket) ? Mesh->GetSocketLocation(RightHandSocket) : Char->GetActorLocation() + FVector(0, 0, 60.0f);
	const FVector Dir = Char->GetActorForwardVector();
	Start += Dir * 60.0f;
	FActorSpawnParameters Params;
	Params.Instigator = Char;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Proj = GetWorld()->SpawnActor<AActor>(ProjClass, Start, Dir.Rotation(), Params);
	if (Proj) {
		Proj->SetActorScale3D(FVector(0.8f + 0.7f * Charge * Strength));
	}
	SpawnReleaseEffect(EWotWeave::Fire, Start);
}

void UWotChannelComponent::WeaveAir(float Charge)
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) {
		return;
	}
	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();
	const float Radius = AirRadius * (0.7f + 0.6f * Charge) * FMath::Max(0.5f, Strength);
	int32 Hit = 0;
	for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
		AWotAICharacter* Target = *It;
		const FVector To = Target->GetActorLocation() - Origin;
		if (To.Size() > Radius) {
			continue;
		}
		// a wall of air in front of the channeler: a wide cone
		if (FVector::DotProduct(To.GetSafeNormal2D(), Forward) < 0.2f) {
			continue;
		}
		FHitResult Fake;
		Fake.ImpactPoint = Target->GetActorLocation();
		Fake.ImpactNormal = -To.GetSafeNormal();
		Fake.Location = Fake.ImpactPoint;
		Fake.TraceStart = Origin;
		Fake.TraceEnd = Target->GetActorLocation();
		UWotGameplayFunctionLibrary::ApplyDirectionalDamage(Owner, Target, AirDamage * Charge * Strength, Fake, AirKnockback * Charge * Strength);
		++Hit;
	}
	SpawnReleaseEffect(EWotWeave::Air, Origin + Forward * 150.0f);
	UE_LOG(LogWotChannel, Log, TEXT("Air weave pushed %d targets"), Hit);
}

void UWotChannelComponent::WeaveWater(float Charge)
{
	AActor* Owner = GetOwner();
	if (UWotAttributeComponent* Attr = Owner ? UWotAttributeComponent::GetAttributes(Owner) : nullptr) {
		Attr->ApplyHealthChange(WaterHeal * Charge * Strength);
	}
	if (Owner) {
		SpawnReleaseEffect(EWotWeave::Water, Owner->GetActorLocation());
	}
}

void UWotChannelComponent::WeaveEarth(float Charge)
{
	AActor* Owner = GetOwner();
	UWotActionComponent* Actions = Owner ? UWotActionComponent::GetActions(Owner) : nullptr;
	if (!Actions || !EarthShieldTag.IsValid() || !GetWorld()) {
		return;
	}
	Actions->ActiveGameplayTags.AddTag(EarthShieldTag);
	GetWorld()->GetTimerManager().SetTimer(ShieldTimer, this, &UWotChannelComponent::EndEarthShield, EarthShieldSeconds * (0.6f + 0.6f * Charge), false);
	if (Owner) {
		SpawnReleaseEffect(EWotWeave::Earth, Owner->GetActorLocation());
	}
	if (AWotCharacter* Char = GetCharacter()) {
		Char->ShowPopupWidget(NSLOCTEXT("Wot", "EarthShield", "Shield of earth"), 1.5f, true);
	}
}

void UWotChannelComponent::EndEarthShield()
{
	AActor* Owner = GetOwner();
	if (UWotActionComponent* Actions = Owner ? UWotActionComponent::GetActions(Owner) : nullptr) {
		Actions->ActiveGameplayTags.RemoveTag(EarthShieldTag);
	}
}

void UWotChannelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelChannel();
	DestroyThreads();
	if (GetWorld()) {
		GetWorld()->GetTimerManager().ClearTimer(ShieldTimer);
		GetWorld()->GetTimerManager().ClearTimer(ThreadFadeTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------- threads from the environment

FLinearColor UWotChannelComponent::ElementColor(EWotWeave Weave, EWotPowerHalf InHalf)
{
	FLinearColor C;
	switch (Weave) {
	case EWotWeave::Fire: C = FLinearColor(1.0f, 0.42f, 0.08f); break;
	case EWotWeave::Air: C = FLinearColor(0.75f, 0.88f, 1.0f); break;
	case EWotWeave::Water: C = FLinearColor(0.18f, 0.5f, 1.0f); break;
	case EWotWeave::Earth: C = FLinearColor(0.85f, 0.58f, 0.2f); break;
	default: C = FLinearColor::White; break;
	}
	// saidar: the clean white light of the show; saidin: a colder, slightly bruised light under the taint
	if (InHalf == EWotPowerHalf::Saidar) {
		C = FMath::Lerp(C, FLinearColor::White, 0.3f);
	} else if (InHalf == EWotPowerHalf::Saidin) {
		C = FMath::Lerp(C, FLinearColor(0.55f, 0.45f, 0.75f), 0.25f);
	}
	return C;
}

static bool WotActorNameHas(const AActor* A, const TArray<FString>& Keys)
{
	FString Text = A->GetName();
#if WITH_EDITOR
	Text += TEXT(" ") + A->GetActorNameOrLabel();
#endif
	TInlineComponentArray<UStaticMeshComponent*> Meshes(A);
	for (const UStaticMeshComponent* M : Meshes) {
		if (M && M->GetStaticMesh()) {
			Text += TEXT(" ") + M->GetStaticMesh()->GetName();
		}
	}
	for (const FString& K : Keys) {
		if (Text.Contains(K, ESearchCase::IgnoreCase)) {
			return true;
		}
	}
	return false;
}

FVector UWotChannelComponent::HandsPoint() const
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	if (Mesh && Mesh->DoesSocketExist(LeftHandSocket) && Mesh->DoesSocketExist(RightHandSocket)) {
		return (Mesh->GetSocketLocation(LeftHandSocket) + Mesh->GetSocketLocation(RightHandSocket)) * 0.5f + Char->GetActorForwardVector() * 12.0f;
	}
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() + FVector(0, 0, 55.0f) + Owner->GetActorForwardVector() * 45.0f : FVector::ZeroVector;
}

void UWotChannelComponent::GatherSources(EWotWeave Weave, const FVector& Origin, int32 Count, TArray<FWotThread>& Out) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner) {
		return;
	}
	FRandomStream Rand(int32(World->GetTimeSeconds() * 1000.0) ^ Owner->GetUniqueID());
	struct FCand { FVector Loc; AActor* Actor; float Dist; };
	TArray<FCand> Cands;
	static const TArray<FString> FireKeys = { TEXT("Fire"), TEXT("Lamp"), TEXT("Torch"), TEXT("Camp"), TEXT("Forge"), TEXT("Brazier"), TEXT("Candle"), TEXT("Hearth"), TEXT("Lantern"), TEXT("Guiding") };
	static const TArray<FString> WaterKeys = { TEXT("Water"), TEXT("Stream"), TEXT("River"), TEXT("Well"), TEXT("Fountain"), TEXT("Reeds"), TEXT("Pond"), TEXT("Lake"), TEXT("Sea"), TEXT("Canal"), TEXT("Snow"), TEXT("Ice") };
	static const TArray<FString> EarthKeys = { TEXT("Boulder"), TEXT("Rock"), TEXT("Stone"), TEXT("Ruin"), TEXT("Wall"), TEXT("Pillar"), TEXT("Obelisk"), TEXT("Stairs"), TEXT("Rubble"), TEXT("Island"), TEXT("Tower"), TEXT("Keep"), TEXT("Ground"), TEXT("Dirt"), TEXT("Sand"), TEXT("Mud"), TEXT("Cobble"), TEXT("Cliff"), TEXT("Terrain") };
	const float MinDist = Weave == EWotWeave::Earth ? 170.0f : Weave == EWotWeave::Water ? 110.0f : 90.0f;
	if (Weave != EWotWeave::Air) {
		// the towns are mostly PCG instances (terrain cubes, lamp posts, wells, reeds, boulders): scan the instanced meshes by mesh + material name
		for (TObjectIterator<UInstancedStaticMeshComponent> It; It; ++It) {
			UInstancedStaticMeshComponent* C = *It;
			if (!C || C->GetWorld() != World || !C->IsRegistered() || C->GetInstanceCount() == 0 || !C->GetStaticMesh()) {
				continue;
			}
			FString Text = C->GetStaticMesh()->GetName();
			for (int32 m = 0; m < C->GetNumMaterials(); ++m) {
				if (UMaterialInterface* M = C->GetMaterial(m)) {
					Text += TEXT(" ") + M->GetName();
				}
			}
			auto Has = [&Text](const TArray<FString>& Keys) {
				for (const FString& K : Keys) {
					if (Text.Contains(K, ESearchCase::IgnoreCase)) {
						return true;
					}
				}
				return false;
			};
			const bool bWater = Has(WaterKeys);
			const bool bMatch = Weave == EWotWeave::Fire ? Has(FireKeys) : Weave == EWotWeave::Water ? bWater : (!bWater && Has(EarthKeys));
			if (!bMatch) {
				continue;
			}
			const FBox MeshBox = C->GetStaticMesh()->GetBoundingBox();
			const FBox CompBox = C->Bounds.GetBox();
			if (CompBox.IsValid && FVector::Dist(CompBox.GetClosestPointTo(Origin), Origin) > SourceSearchRadius) {
				continue;
			}
			for (int32 i = 0; i < C->GetInstanceCount(); ++i) {
				FTransform T;
				if (!C->GetInstanceTransform(i, T, true)) {
					continue;
				}
				const FVector L = T.GetLocation();
				if (FMath::Abs(L.X - Origin.X) > SourceSearchRadius || FMath::Abs(L.Y - Origin.Y) > SourceSearchRadius) {
					continue;
				}
				const FBox B = MeshBox.TransformBy(T);
				const FVector P(B.GetCenter().X, B.GetCenter().Y, B.Max.Z + 4.0f);   // the top of the thing: lamp head, well rim, water surface, cube top
				const float D = FVector::Dist(P, Origin);
				if (D > SourceSearchRadius || D < MinDist) {
					continue;
				}
				Cands.Add({ P, C->GetOwner(), D });
			}
		}
		for (TActorIterator<AActor> It(World); It; ++It) {
			AActor* A = *It;
			if (A == Owner || A->IsA<APawn>() || A->IsA<AWotTiedWeave>()) {
				continue;
			}
			const FBox B = A->GetComponentsBoundingBox(true);
			if (!B.IsValid) {
				continue;
			}
			FVector P = B.GetClosestPointTo(Origin);
			const float D = FVector::Dist(P, Origin);
			if (D > SourceSearchRadius || D < MinDist) {
				continue;
			}
			bool bMatch = false;
			if (Weave == EWotWeave::Fire) {
				if (const ULocalLightComponent* L = A->FindComponentByClass<ULocalLightComponent>()) {
					if (L->Intensity > 0.0f) {
						P = L->GetComponentLocation();
						bMatch = true;
					}
				}
				bMatch = bMatch || WotActorNameHas(A, FireKeys);
				if (!bMatch) {
					continue;
				}
			} else if (Weave == EWotWeave::Water) {
				if (!WotActorNameHas(A, WaterKeys)) {
					continue;
				}
				P.Z = B.Max.Z + 6.0f;   // the surface
			} else {
				if (!WotActorNameHas(A, EarthKeys)) {
					continue;
				}
			}
			Cands.Add({ P, A, FVector::Dist(P, Origin) });
		}
	}
	Cands.Sort([](const FCand& L, const FCand& R) { return L.Dist < R.Dist; });
	TArray<float> UsedAngles;
	for (const FCand& C : Cands) {
		if (Out.Num() >= Count) {
			break;
		}
		const float Ang = FMath::Atan2(C.Loc.Y - Origin.Y, C.Loc.X - Origin.X);
		bool bTooClose = false;
		for (float U : UsedAngles) {
			if (FMath::Abs(FMath::FindDeltaAngleRadians(U, Ang)) < FMath::DegreesToRadians(18.0f)) {
				bTooClose = true;
				break;
			}
		}
		if (bTooClose) {
			continue;
		}
		UsedAngles.Add(Ang);
		FWotThread T;
		T.Source = C.Loc;
		T.bFromEnvironment = true;
		if (C.Actor && C.Actor->GetRootComponent() && C.Actor->GetRootComponent()->Mobility == EComponentMobility::Movable && !C.Actor->FindComponentByClass<UInstancedStaticMeshComponent>()) {
			T.SourceActor = C.Actor;   // a carried lantern, a cart: follow it
			T.SourceOffset = C.Loc - C.Actor->GetActorLocation();
		}
		Out.Add(T);
	}
	// fill the rest from what is always there: embers from the ground, mist in the air, the ground itself, the sky
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotWeaveSource), false, Owner);
	int32 Guard = 0;
	while (Out.Num() < Count && Guard++ < 40) {
		const float Ang = Rand.FRandRange(0.0f, 2.0f * PI);
		FWotThread T;
		if (Weave == EWotWeave::Air) {
			const float R = Rand.FRandRange(260.0f, 620.0f);
			T.Source = Origin + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, Rand.FRandRange(160.0f, 420.0f));
			T.Drift = FVector(Rand.FRandRange(-70.0f, 70.0f), Rand.FRandRange(-70.0f, 70.0f), Rand.FRandRange(-15.0f, 25.0f));
			T.Bob = Rand.FRandRange(0.0f, 6.28f);
		} else if (Weave == EWotWeave::Water) {
			const float R = Rand.FRandRange(180.0f, 420.0f);
			T.Source = Origin + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, Rand.FRandRange(90.0f, 240.0f));
			T.Drift = FVector(Rand.FRandRange(-25.0f, 25.0f), Rand.FRandRange(-25.0f, 25.0f), Rand.FRandRange(-10.0f, 10.0f));
			T.Bob = Rand.FRandRange(0.0f, 6.28f);
		} else {
			const float R = Rand.FRandRange(220.0f, 520.0f);
			const FVector P = Origin + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 0);
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 260.0f), P - FVector(0, 0, 500.0f), ECC_Visibility, Params)) {
				T.Source = Hit.ImpactPoint + FVector(0, 0, Weave == EWotWeave::Fire ? 8.0f : 2.0f);
			} else {
				T.Source = P - FVector(0, 0, 90.0f);
			}
			T.Bob = Rand.FRandRange(0.0f, 6.28f);
		}
		Out.Add(T);
	}
}

void UWotChannelComponent::SpawnThreads()
{
	DestroyThreads();
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	UNiagaraSystem* System = ThreadSystem.LoadSynchronous();
	if (!World || !Owner || !System) {
		return;
	}
	const FVector Origin = Owner->GetActorLocation();
	const int32 Count = FMath::Max(1, ThreadCount + (SelectedWeave == EWotWeave::Air ? 1 : 0));
	GatherSources(SelectedWeave, Origin, Count, Threads);
	const int32 EnvCount = (int32)Algo::CountIf(Threads, [](const FWotThread& T) { return T.bFromEnvironment; });
	if (SelectedWeave == EWotWeave::Water) {
		// Water and Spirit: one thread rises from the channeler's own heart
		FWotThread Spirit;
		Spirit.SourceActor = Owner;
		Spirit.SourceOffset = FVector(0, 0, 30.0f);
		Spirit.Source = Origin + Spirit.SourceOffset;
		Threads.Add(Spirit);
	}
	const FVector Target = HandsPoint();
	const FLinearColor Color = ElementColor(SelectedWeave, Half);
	FRandomStream Rand(Owner->GetUniqueID() + int32(World->GetTimeSeconds() * 37.0));
	for (int32 i = 0; i < Threads.Num(); ++i) {
		FWotThread& T = Threads[i];
		UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, T.Source, FRotator::ZeroRotator, FVector(1.0f), false, true, ENCPoolMethod::None, true);
		if (!Comp) {
			continue;
		}
		const bool bSpirit = T.SourceActor.Get() == Owner;
		Comp->SetVariableVec3(FName("User.Source"), T.Source);
		Comp->SetVariableVec3(FName("User.Target"), Target);
		Comp->SetVariableLinearColor(FName("User.Color"), bSpirit ? FLinearColor::White : Color);
		Comp->SetVariableFloat(FName("User.Width"), ThreadWidth * 0.5f);
		Comp->SetVariableFloat(FName("User.Phase"), Rand.FRandRange(0.0f, 6.28f));
		Comp->SetVariableFloat(FName("User.Amplitude"), bSpirit ? 12.0f : Rand.FRandRange(26.0f, 44.0f));
		Comp->SetVariableFloat(FName("User.Gather"), 0.0f);
		T.Comp = Comp;
		ThreadComps.Add(Comp);
	}
	UE_LOG(LogWotChannel, Log, TEXT("%s draws %d %s threads (%d from the environment)"), *GetNameSafe(Owner), Threads.Num(), *WeaveName(SelectedWeave).ToString(), EnvCount);
}

void UWotChannelComponent::UpdateThreads(float DeltaTime)
{
	if (Threads.Num() == 0) {
		return;
	}
	const FVector Target = HandsPoint();
	const float Charge = GetCharge();
	const float Width = ThreadWidth * (0.45f + 0.75f * Charge) * FMath::Sqrt(FMath::Max(0.2f, Strength));
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	for (FWotThread& T : Threads) {
		UNiagaraComponent* Comp = T.Comp.Get();
		if (!Comp) {
			continue;
		}
		if (T.SourceActor.IsValid()) {
			T.Source = T.SourceActor->GetActorLocation() + T.SourceOffset;
		} else {
			T.Source += T.Drift * DeltaTime;
			T.Source.Z += FMath::Sin(Now * 1.7 + T.Bob) * 6.0f * DeltaTime;
		}
		Comp->SetVariableVec3(FName("User.Source"), T.Source);
		Comp->SetVariableVec3(FName("User.Target"), Target);
		Comp->SetVariableFloat(FName("User.Width"), Width);
		Comp->SetVariableFloat(FName("User.Gather"), Charge);
	}
}

void UWotChannelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bChanneling) {
		UpdateThreads(DeltaTime);
	}
}

void UWotChannelComponent::ReleaseThreads(const FVector& To)
{
	for (FWotThread& T : Threads) {
		if (UNiagaraComponent* Comp = T.Comp.Get()) {
			Comp->SetVariableVec3(FName("User.Target"), To);
			Comp->SetVariableFloat(FName("User.Gather"), 1.0f);
			Comp->SetVariableFloat(FName("User.Amplitude"), 10.0f);
			Comp->SetVariableFloat(FName("User.Width"), ThreadWidth * 1.4f);
		}
	}
	Threads.Reset();
	if (GetWorld()) {
		GetWorld()->GetTimerManager().SetTimer(ThreadFadeTimer, this, &UWotChannelComponent::DestroyThreads, 0.3f, false);
	}
}

void UWotChannelComponent::DestroyThreads()
{
	for (UNiagaraComponent* Comp : ThreadComps) {
		if (Comp) {
			Comp->Deactivate();
			Comp->DestroyComponent();
		}
	}
	ThreadComps.Reset();
	Threads.Reset();
}

// ---------------------------------------------------------------- tying off

bool UWotChannelComponent::TieOffWeave()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!bChanneling || !Owner || !World || !TiedWeaveClass) {
		return false;
	}
	const float Charge = GetCharge();
	if (Charge < 0.2f) {
		if (AWotCharacter* Char = GetCharacter()) {
			Char->ShowPopupWidget(NSLOCTEXT("Wot", "TieTooSoon", "The weave is too thin to tie"), 1.0f, false);
		}
		return false;
	}
	// in front of the channeler, on the ground
	FVector At = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 260.0f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotTieOff), false, Owner);
	if (World->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 150.0f), At - FVector(0, 0, 400.0f), ECC_Visibility, Params)) {
		At = Hit.ImpactPoint;
	} else {
		At.Z = Owner->GetActorLocation().Z - 90.0f;
	}
	FActorSpawnParameters SP;
	SP.Instigator = Cast<APawn>(Owner);
	SP.Owner = Owner;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWotTiedWeave* Knot = World->SpawnActorDeferred<AWotTiedWeave>(TiedWeaveClass, FTransform(FRotator(0, Owner->GetActorRotation().Yaw, 0), At), Owner, SP.Instigator, SP.SpawnCollisionHandlingOverride);
	if (!Knot) {
		return false;
	}
	const float Power = (0.35f + 0.65f * Charge) * FMath::Max(0.2f, Strength);
	Knot->Configure(SelectedWeave, Half, Power, ElementColor(SelectedWeave, Half), TiedWeaveSeconds * (0.6f + 0.8f * Charge));
	Knot->FinishSpawning(FTransform(FRotator(0, Owner->GetActorRotation().Yaw, 0), At));
	TiedWeaves.RemoveAll([](const TWeakObjectPtr<AWotTiedWeave>& W) { return !W.IsValid(); });
	TiedWeaves.Add(Knot);
	while (TiedWeaves.Num() > MaxTiedWeaves) {
		if (AWotTiedWeave* Old = TiedWeaves[0].Get()) {
			Old->Unravel();
		}
		TiedWeaves.RemoveAt(0);
	}
	// the channeler lets go: the threads leap into the knot and the Power is released without a weave
	bChanneling = false;
	StopEffects();
	ReleaseThreads(At + FVector(0, 0, 100.0f));
	SetComponentTickEnabled(false);
	if (AWotCharacter* Char = GetCharacter()) {
		Char->ShowPopupWidget(FText::Format(NSLOCTEXT("Wot", "TiedOff", "Tied off: {0}"), WeaveName(SelectedWeave)), 1.5f, true);
	}
	UE_LOG(LogWotChannel, Log, TEXT("%s ties off %s at charge %.2f (power %.2f, %.0fs)"), *GetNameSafe(Owner), *WeaveName(SelectedWeave).ToString(), Charge, Power, Knot->Duration);
	return true;
}

// ---------------------------------------------------------------- console

static UWotChannelComponent* WotLocalChannel(UWorld* World)
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AWotCharacter* Char = PC ? Cast<AWotCharacter>(PC->GetPawn()) : nullptr;
	return Char ? Char->GetChannelComponent() : nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GWotChannelWeave(
	TEXT("wot.Channel.Weave"), TEXT("Select a weave: wot.Channel.Weave Fire|Air|Water|Earth"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotChannelComponent* C = WotLocalChannel(World);
		if (!C || Args.Num() < 1) {
			return;
		}
		const FString W = Args[0].ToLower();
		C->SelectWeave(W == TEXT("air") ? EWotWeave::Air : W == TEXT("water") ? EWotWeave::Water : W == TEXT("earth") ? EWotWeave::Earth : EWotWeave::Fire);
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotChannelBegin(
	TEXT("wot.Channel.Begin"), TEXT("Start channeling (hold)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotChannelComponent* C = WotLocalChannel(World)) {
			C->BeginChannel();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotChannelRelease(
	TEXT("wot.Channel.Release"), TEXT("Release the weave being channeled"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotChannelComponent* C = WotLocalChannel(World)) {
			C->ReleaseWeave();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotChannelHalf(
	TEXT("wot.Channel.Half"), TEXT("Set the half of the Power: wot.Channel.Half saidin|saidar|none"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotChannelComponent* C = WotLocalChannel(World);
		if (!C || Args.Num() < 1) {
			return;
		}
		const FString H = Args[0].ToLower();
		C->Half = H == TEXT("saidar") ? EWotPowerHalf::Saidar : H == TEXT("none") ? EWotPowerHalf::None : EWotPowerHalf::Saidin;
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotChannelTie(
	TEXT("wot.Channel.Tie"), TEXT("Tie off the weave being channeled so it persists"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		if (UWotChannelComponent* C = WotLocalChannel(World)) {
			C->TieOffWeave();
		}
	}));
