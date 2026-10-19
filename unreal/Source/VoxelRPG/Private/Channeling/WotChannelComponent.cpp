#include "Channeling/WotChannelComponent.h"
#include "Channeling/WotTiedWeave.h"
#include "Channeling/WotChannelCameraShake.h"
#include "Save/WotSaveSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	// saidin: shadow for the taint, lightning for the clean torrent, engine primitives for the ooze
	TaintTrailSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Shadow/NS_Shadow_Trail_Purple.NS_Shadow_Trail_Purple")));
	TaintBurstSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Shadow/NS_Shadow_Impact_Purple.NS_Shadow_Impact_Purple")));
	SurgeTrailSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Lightning/NS_Lightning_Trail_Blue_Fix.NS_Lightning_Trail_Blue_Fix")));
	SurgeBurstSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Lightning/NS_Lightning_Muzzle_Blue_Fix.NS_Lightning_Muzzle_Blue_Fix")));
	SaidinShakeClass = UWotChannelCameraShake::StaticClass();
	OozeMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	OozeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
}

bool UWotChannelComponent::IsTainted() const
{
	return Half == EWotPowerHalf::Saidin && !UWotSaveSubsystem::IsSaidinCleansedIn(this);
}

APlayerController* UWotChannelComponent::LocalPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return PC && PC->IsLocalController() ? PC : nullptr;
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
	FText Seize = NSLOCTEXT("Wot", "Embrace", "Embracing saidar");
	if (Half == EWotPowerHalf::Saidin) {
		BeginSaidin();
		Seize = bChanneledTainted ? NSLOCTEXT("Wot", "SeizeTainted", "Seizing saidin, the taint oozing over it") : NSLOCTEXT("Wot", "SeizeClean", "Seizing saidin, a torrent of fire and ice");
	}
	Char->ShowPopupWidget(FText::Format(NSLOCTEXT("Wot", "Channeling", "{0} ... {1}"), Seize, WeaveName(SelectedWeave)), 1.2f, false);
	UE_LOG(LogWotChannel, Log, TEXT("%s begins channeling (%s%s, weave %s)"), *GetNameSafe(Char), Half == EWotPowerHalf::Saidar ? TEXT("saidar") : TEXT("saidin"), Half == EWotPowerHalf::Saidin ? (bChanneledTainted ? TEXT(", tainted") : TEXT(", cleansed")) : TEXT(""), *WeaveName(SelectedWeave).ToString());
}

void UWotChannelComponent::CancelChannel()
{
	if (!bChanneling) {
		return;
	}
	bChanneling = false;
	StopEffects();
	DestroyThreads();
	StopSaidin(false, FVector::ZeroVector);
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
		const FVector At = bOutward ? Owner->GetActorLocation() + Owner->GetActorForwardVector() * 260.0f + FVector(0, 0, 70.0f) : Owner->GetActorLocation() + FVector(0, 0, 30.0f);
		ReleaseThreads(At);
		StopSaidin(true, At);
	}
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
	DestroyOoze();
	if (UWotChannelCameraShake* S = Shake.Get()) {
		if (APlayerController* PC = LocalPlayerController()) {
			if (PC->PlayerCameraManager) {
				PC->PlayerCameraManager->StopCameraShake(S, true);
			}
		}
		Shake = nullptr;
	}
	if (GetWorld()) {
		GetWorld()->GetTimerManager().ClearTimer(ShieldTimer);
		GetWorld()->GetTimerManager().ClearTimer(ThreadFadeTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------- threads from the environment

FLinearColor UWotChannelComponent::ElementColor(EWotWeave Weave, EWotPowerHalf InHalf)
{
	return ElementColorFor(Weave, InHalf, false);
}

FLinearColor UWotChannelComponent::ElementColorFor(EWotWeave Weave, EWotPowerHalf InHalf, bool bSaidinCleansed)
{
	FLinearColor C;
	switch (Weave) {
	case EWotWeave::Fire: C = FLinearColor(1.0f, 0.42f, 0.08f); break;
	case EWotWeave::Air: C = FLinearColor(0.75f, 0.88f, 1.0f); break;
	case EWotWeave::Water: C = FLinearColor(0.18f, 0.5f, 1.0f); break;
	case EWotWeave::Earth: C = FLinearColor(0.85f, 0.58f, 0.2f); break;
	default: C = FLinearColor::White; break;
	}
	if (InHalf == EWotPowerHalf::Saidar) {
		// the clean white light of the show
		C = FMath::Lerp(C, FLinearColor::White, 0.3f);
	} else if (InHalf == EWotPowerHalf::Saidin && !bSaidinCleansed) {
		// the taint: the element drowned in violet (the ribbons are additive, so the black itself comes from the ooze,
		// the shadow on the hands and the darkened screen; the threads flicker down toward black instead)
		C = FMath::Lerp(C, FLinearColor(0.5f, 0.05f, 0.85f), 0.75f) * 1.15f;
	} else if (InHalf == EWotPowerHalf::Saidin) {
		// cleansed: fire and ice, a cold white-blue blaze brighter than saidar
		C = FMath::Lerp(C, FLinearColor(0.85f, 0.95f, 1.0f), 0.45f) * 1.35f;
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
	const FLinearColor Color = ElementColorFor(SelectedWeave, Half, !IsTainted());
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
		T.Amplitude = bSpirit ? 12.0f : Rand.FRandRange(26.0f, 44.0f);
		Comp->SetVariableFloat(FName("User.Amplitude"), T.Amplitude);
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
	const bool bSaidin = IsSaidin();
	const bool bTaint = bSaidin && bChanneledTainted;
	const bool bSurge = bSaidin && (float)Now < SurgeUntil;
	const FLinearColor Base = ElementColorFor(SelectedWeave, Half, !bTaint);
	const FLinearColor Dark(0.02f, 0.0f, 0.04f), Bruise(0.2f, 0.0f, 0.28f);
	int32 Index = 0;
	for (FWotThread& T : Threads) {
		UNiagaraComponent* Comp = T.Comp.Get();
		if (!Comp) {
			++Index;
			continue;
		}
		if (T.SourceActor.IsValid()) {
			T.Source = T.SourceActor->GetActorLocation() + T.SourceOffset;
		} else {
			T.Source += T.Drift * DeltaTime;
			T.Source.Z += FMath::Sin(Now * 1.7 + T.Bob) * 6.0f * DeltaTime;
		}
		FVector ThreadTarget = Target;
		float ThreadWidthNow = Width;
		if (bSaidin) {
			// saidin fights the channeler: the threads thrash (the hands point jitters per thread), the ribbons wave wider,
			// their width stutters; the taint also drowns them in flickering black, the clean torrent flares white on a surge
			const float N1 = FMath::PerlinNoise1D(Now * (bTaint ? 5.0 : 9.0) + Index * 17.3), N2 = FMath::PerlinNoise1D(Now * (bTaint ? 4.0 : 8.0) + Index * 29.1 + 50.0), N3 = FMath::PerlinNoise1D(Now * 7.0 + Index * 13.7 + 120.0);
			const float Jitter = (bTaint ? 10.0f : 16.0f) * Chaos;
			ThreadTarget += FVector(N1, N2, N3) * Jitter;
			ThreadWidthNow *= 1.5f * (0.65f + 0.7f * FMath::Abs(N3) + (bSurge ? 0.6f : 0.0f));   // saidin's ribbons run thicker: the game camera sits far above
			Comp->SetVariableFloat(FName("User.Amplitude"), T.Amplitude * (1.0f + (bTaint ? 0.8f : 1.3f) * Chaos));
			FLinearColor C = Base;
			if (bTaint) {
				// violet most of the time, guttering down to bruise-dark, and a black dropout on each surge
				const float Drown = FMath::Clamp(0.5f + 0.5f * N1, 0.0f, 1.0f);
				C = bSurge ? FMath::Lerp(Base, Dark, 0.85f) : FMath::Lerp(Base, Bruise, Drown * 0.6f);
			} else if (bSurge) {
				C = FMath::Lerp(Base, FLinearColor(1.0f, 1.0f, 1.0f) * 2.2f, 0.7f);
			}
			Comp->SetVariableLinearColor(FName("User.Color"), C);
		}
		Comp->SetVariableVec3(FName("User.Source"), T.Source);
		Comp->SetVariableVec3(FName("User.Target"), ThreadTarget);
		Comp->SetVariableFloat(FName("User.Width"), ThreadWidthNow);
		Comp->SetVariableFloat(FName("User.Gather"), Charge);
		++Index;
	}
}

void UWotChannelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const bool bHoldingSaidin = bChanneling && IsSaidin();
	if (bHoldingSaidin) {
		UpdateSaidin(DeltaTime);
	}
	if (bChanneling) {
		UpdateThreads(DeltaTime);
	}
	UpdateOoze(DeltaTime);
	UpdateCamera(DeltaTime, bHoldingSaidin);
	const float Now = GetWorld() ? (float)GetWorld()->GetTimeSeconds() : 0.0f;
	if (!bChanneling && Ooze.Num() == 0 && !Shake.IsValid() && Now >= ReleaseKickUntil) {
		SetComponentTickEnabled(false);
	}
}

// ---------------------------------------------------------------- saidin: the taint, the torrent

void UWotChannelComponent::BeginSaidin()
{
	bChanneledTainted = IsTainted();
	Chaos = 0.5f;
	const float Now = GetWorld() ? (float)GetWorld()->GetTimeSeconds() : 0.0f;
	SurgeUntil = 0.0f;
	NextSurgeTime = Now + FMath::FRandRange(0.5f, 1.2f);
	OozeAccumulator = 0.0f;
	if (bChanneledTainted) {
		AttachEffect(TaintSystem, LeftHandSocket, true);
		AttachEffect(TaintTrailSystem, LeftHandSocket, true);
		AttachEffect(TaintTrailSystem, RightHandSocket, true);
		// the shadow pools around the feet too: large enough to read from the game's high camera
		AttachEffectScaled(TaintTrailSystem, FVector(0, 0, 14.0f), 2.8f);
	} else {
		AttachEffect(SurgeTrailSystem, LeftHandSocket, true);
		AttachEffect(SurgeTrailSystem, RightHandSocket, true);
		AttachEffectScaled(SurgeTrailSystem, FVector(0, 0, 60.0f), 2.2f);
	}
	SetComponentTickEnabled(true);
}

UNiagaraComponent* UWotChannelComponent::AttachEffectScaled(const TSoftObjectPtr<UNiagaraSystem>& SoftSystem, const FVector& Offset, float Scale)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	UNiagaraSystem* System = SoftSystem.LoadSynchronous();
	if (!Mesh || !System) {
		return nullptr;
	}
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Mesh, NAME_None, Offset, FRotator::ZeroRotator, FVector(Scale), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None, true, true);
	if (Comp) {
		ActiveEffects.Add(Comp);
	}
	return Comp;
}

void UWotChannelComponent::UpdateSaidin(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World) {
		return;
	}
	const float Now = (float)World->GetTimeSeconds();
	const float Charge = GetCharge();
	// a restless baseline that never settles, plus surges: the Power trying to burn the channeler out
	const float Floor = bChanneledTainted ? 0.3f : 0.4f;
	Chaos = Floor + (1.0f - Floor) * FMath::Abs(FMath::PerlinNoise1D(Now * (bChanneledTainted ? 1.3f : 2.4f) + GetUniqueID() * 0.01f)) * (0.6f + 0.4f * Charge);
	if (Now >= NextSurgeTime) {
		SurgeUntil = Now + (bChanneledTainted ? 0.45f : 0.25f);
		NextSurgeTime = Now + FMath::FRandRange(0.8f, 2.2f) * (bChanneledTainted ? 1.3f : 1.0f) / (0.6f + 0.6f * Charge);
		const FVector Hands = HandsPoint();
		UNiagaraSystem* Burst = (bChanneledTainted ? TaintBurstSystem : SurgeBurstSystem).LoadSynchronous();
		if (Burst) {
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Burst, Hands, GetOwner()->GetActorRotation(), FVector(bChanneledTainted ? 1.1f : 1.3f), true, true);
		}
		if (bChanneledTainted) {
			for (int32 i = 0; i < 3; ++i) {
				SpawnOozeDrop(Hands + FVector(FMath::FRandRange(-12.0f, 12.0f), FMath::FRandRange(-12.0f, 12.0f), FMath::FRandRange(-6.0f, 6.0f)));
			}
		}
	}
	if (Now < SurgeUntil) {
		Chaos = FMath::Max(Chaos, 1.4f);
	}
	if (bChanneledTainted && OozeRate > 0.0f) {
		OozeAccumulator += DeltaTime * OozeRate * (0.35f + 0.65f * Charge);
		ACharacter* Char = Cast<ACharacter>(GetOwner());
		USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
		while (OozeAccumulator >= 1.0f) {
			OozeAccumulator -= 1.0f;
			const FName Socket = FMath::RandBool() ? LeftHandSocket : RightHandSocket;
			const FVector From = Mesh && Mesh->DoesSocketExist(Socket) ? Mesh->GetSocketLocation(Socket) : HandsPoint();
			SpawnOozeDrop(From + FVector(FMath::FRandRange(-8.0f, 8.0f), FMath::FRandRange(-8.0f, 8.0f), -4.0f));
		}
	}
}

void UWotChannelComponent::StopSaidin(bool bLoosed, const FVector& LoosedAt)
{
	if (!IsSaidin() && !Shake.IsValid()) {
		return;
	}
	UWorld* World = GetWorld();
	if (World && bLoosed && IsSaidin()) {
		// the release: a burst of the half's nature at the point the weave leaves, and a kick of the camera
		UNiagaraSystem* Burst = (bChanneledTainted ? TaintBurstSystem : SurgeBurstSystem).LoadSynchronous();
		if (Burst) {
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Burst, LoosedAt, GetOwner()->GetActorRotation(), FVector(1.6f + 0.8f * GetCharge()), true, true);
		}
		if (bChanneledTainted) {
			for (int32 i = 0; i < 6; ++i) {
				SpawnOozeDrop(HandsPoint() + FVector(FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), 0.0f));
			}
		}
		ReleaseKickUntil = (float)World->GetTimeSeconds() + 0.35f;
	}
	Chaos = 0.0f;
	SurgeUntil = 0.0f;
	// the shake fades out and the ooze sinks away on their own: the tick keeps running until both are gone
	SetComponentTickEnabled(true);
}

void UWotChannelComponent::SpawnOozeDrop(const FVector& From)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	UStaticMesh* Mesh = OozeMesh.LoadSynchronous();
	if (!World || !Owner || !Mesh) {
		return;
	}
	if (Ooze.Num() >= MaxOozeDrops) {
		// recycle the oldest drop that has already landed
		int32 Oldest = INDEX_NONE;
		for (int32 i = 0; i < Ooze.Num(); ++i) {
			if (Ooze[i].bLanded && (Oldest == INDEX_NONE || Ooze[i].Age > Ooze[Oldest].Age)) {
				Oldest = i;
			}
		}
		if (Oldest == INDEX_NONE) {
			return;
		}
		if (UStaticMeshComponent* C = Ooze[Oldest].Comp.Get()) {
			C->DestroyComponent();
		}
		Ooze.RemoveAt(Oldest);
	}
	if (!OozeMID) {
		if (UMaterialInterface* Mat = OozeMaterial.LoadSynchronous()) {
			OozeMID = UMaterialInstanceDynamic::Create(Mat, this);
			OozeMID->SetVectorParameterValue(FName("Color"), OozeColor);
		}
	}
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
	Comp->SetStaticMesh(Mesh);
	if (OozeMID) {
		Comp->SetMaterial(0, OozeMID);
	}
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Comp->SetCanEverAffectNavigation(false);
	Comp->RegisterComponent();
	FWotOozeDrop Drop;
	Drop.Comp = Comp;
	Drop.Size = FMath::FRandRange(18.0f, 32.0f);
	Drop.Velocity = FVector(FMath::FRandRange(-30.0f, 30.0f), FMath::FRandRange(-30.0f, 30.0f), FMath::FRandRange(-10.0f, 20.0f));
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WotOoze), false, Owner);
	Drop.GroundZ = World->LineTraceSingleByChannel(Hit, From, From - FVector(0, 0, 700.0f), ECC_Visibility, Params) ? Hit.ImpactPoint.Z : From.Z - 120.0f;
	Drop.Age = -1.0f;
	Comp->SetWorldLocation(From);
	Comp->SetWorldScale3D(FVector(Drop.Size, Drop.Size, Drop.Size * 1.5f) / 100.0f);   // the sphere is 100 cm across
	Ooze.Add(Drop);
}

void UWotChannelComponent::UpdateOoze(float DeltaTime)
{
	for (int32 i = Ooze.Num() - 1; i >= 0; --i) {
		FWotOozeDrop& D = Ooze[i];
		UStaticMeshComponent* Comp = D.Comp.Get();
		if (!Comp) {
			Ooze.RemoveAt(i);
			continue;
		}
		if (!D.bLanded) {
			// thick: it falls slowly, stretching as it goes
			D.Velocity.Z -= 560.0f * DeltaTime;
			D.Velocity.X *= 1.0f - 2.0f * DeltaTime;
			D.Velocity.Y *= 1.0f - 2.0f * DeltaTime;
			FVector Loc = Comp->GetComponentLocation() + D.Velocity * DeltaTime;
			const float Stretch = FMath::Clamp(1.2f + FMath::Abs(D.Velocity.Z) / 400.0f, 1.2f, 2.6f);
			if (Loc.Z - D.Size * 0.5f <= D.GroundZ) {
				// the splat: a flat dark puddle
				D.bLanded = true;
				D.Age = 0.0f;
				Loc.Z = D.GroundZ + D.Size * 0.06f;
				Comp->SetWorldLocation(Loc);
				Comp->SetWorldScale3D(FVector(D.Size * 3.0f, D.Size * 3.0f, D.Size * 0.24f) / 100.0f);
			} else {
				Comp->SetWorldLocation(Loc);
				Comp->SetWorldScale3D(FVector(D.Size, D.Size, D.Size * Stretch) / 100.0f);
			}
			continue;
		}
		D.Age += DeltaTime;
		// the puddle spreads a little, then sinks into the ground over a few seconds
		const float Spread = 1.0f + 0.4f * FMath::Min(D.Age, 1.0f);
		const float Hold = 2.6f, Sink = 2.0f;
		const float Fade = D.Age <= Hold ? 1.0f : 1.0f - (D.Age - Hold) / Sink;
		if (Fade <= 0.0f) {
			Comp->DestroyComponent();
			Ooze.RemoveAt(i);
			continue;
		}
		Comp->SetWorldScale3D(FVector(D.Size * 3.0f * Spread * (0.4f + 0.6f * Fade), D.Size * 3.0f * Spread * (0.4f + 0.6f * Fade), D.Size * 0.24f * Fade) / 100.0f);
	}
}

void UWotChannelComponent::DestroyOoze()
{
	for (FWotOozeDrop& D : Ooze) {
		if (UStaticMeshComponent* C = D.Comp.Get()) {
			C->DestroyComponent();
		}
	}
	Ooze.Reset();
}

void UWotChannelComponent::UpdateCamera(float DeltaTime, bool bHolding)
{
	APlayerController* PC = LocalPlayerController();
	APlayerCameraManager* Cam = PC ? PC->PlayerCameraManager : nullptr;
	if (!Cam || !GetWorld()) {
		return;
	}
	const float Now = (float)GetWorld()->GetTimeSeconds();
	const float Kick = Now < ReleaseKickUntil ? (ReleaseKickUntil - Now) / 0.35f : 0.0f;   // 1 -> 0 after a release
	const float Charge = GetCharge();
	// the shake (a tremor while saidin is held, a lurch when it is loosed, nothing for saidar) also carries the
	// post-process: the camera manager clears plain cached blends before the view is built, a shake's land every frame
	if (!SaidinShakeClass) {
		return;
	}
	if (bHolding && !Shake.IsValid()) {
		Shake = Cast<UWotChannelCameraShake>(Cam->StartCameraShake(SaidinShakeClass, 1.0f));
	}
	UWotChannelCameraShake* S = Shake.Get();
	UWotChannelShakePattern* Pattern = S ? Cast<UWotChannelShakePattern>(S->GetRootShakePattern()) : nullptr;   // the live (instanced) pattern
	if (!Pattern) {
		return;
	}
	// clean saidin is the harder to hold: a fast, strong tremor that spikes on every surge; the taint is a slower lurch
	const float Level = bHolding ? Chaos * (0.35f + 0.65f * Charge) : 0.0f;
	Pattern->Chaos = FMath::Max(Level, Kick * 1.7f) * SaidinShakeScale;
	Pattern->Frequency = bChanneledTainted ? 4.5f : 12.0f;
	Pattern->RotationDegrees = bChanneledTainted ? 1.3f : 1.9f;
	Pattern->FOVDegrees = bChanneledTainted ? 0.9f : 1.2f;
	Pattern->PostProcessWeight = 0.0f;
	// the screen: the taint drains colour and closes in at the edges, the clean torrent blooms and fringes
	if (SaidinScreenEffect > 0.0f && (bHolding || Kick > 0.0f)) {
		FPostProcessSettings PP;
		float Weight;
		if (bChanneledTainted) {
			PP.bOverride_ColorSaturation = true; PP.ColorSaturation = FVector4(0.45f, 0.45f, 0.45f, 1.0f);
			PP.bOverride_SceneColorTint = true; PP.SceneColorTint = FLinearColor(0.78f, 0.7f, 0.95f);
			PP.bOverride_VignetteIntensity = true; PP.VignetteIntensity = 1.1f;
			PP.bOverride_FilmGrainIntensity = true; PP.FilmGrainIntensity = 0.75f;
			PP.bOverride_SceneFringeIntensity = true; PP.SceneFringeIntensity = 2.5f * Chaos;
			PP.bOverride_ColorGamma = true; PP.ColorGamma = FVector4(0.9f, 0.86f, 0.98f, 1.0f);
			Weight = 0.35f + 0.6f * Charge;
		} else {
			// a cold, over-bright glare that pulses with the chaos
			const float Glare = 1.0f + 0.25f * Chaos;
			PP.bOverride_BloomIntensity = true; PP.BloomIntensity = 2.4f;
			PP.bOverride_SceneFringeIntensity = true; PP.SceneFringeIntensity = 3.0f * Chaos;
			PP.bOverride_ColorSaturation = true; PP.ColorSaturation = FVector4(1.2f, 1.2f, 1.2f, 1.0f);
			PP.bOverride_ColorContrast = true; PP.ColorContrast = FVector4(1.12f, 1.12f, 1.12f, 1.0f);
			PP.bOverride_ColorGain = true; PP.ColorGain = FVector4(1.0f * Glare, 1.06f * Glare, 1.22f * Glare, 1.0f);
			PP.bOverride_SceneColorTint = true; PP.SceneColorTint = FLinearColor(0.9f, 0.96f, 1.12f);
			PP.bOverride_VignetteIntensity = true; PP.VignetteIntensity = 0.55f;
			Weight = 0.3f + 0.6f * Charge;
		}
		Weight *= SaidinScreenEffect * (bHolding ? 1.0f : Kick);
		Pattern->PostProcess = PP;
		Pattern->PostProcessWeight = Weight;
	}
	if (!bHolding && Kick <= 0.0f) {
		Cam->StopCameraShake(S, false);
		Shake = nullptr;
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
	Knot->Configure(SelectedWeave, Half, Power, ElementColorFor(SelectedWeave, Half, !IsTainted()), TiedWeaveSeconds * (0.6f + 0.8f * Charge), IsTainted());
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
	StopSaidin(true, At + FVector(0, 0, 100.0f));
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

static FAutoConsoleCommandWithWorldAndArgs GWotChannelCleanse(
	TEXT("wot.Channel.Cleanse"), TEXT("Set whether saidin has been cleansed (saved with the game): wot.Channel.Cleanse 1|0"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotSaveSubsystem* Save = UWotSaveSubsystem::Get(World);
		if (!Save) {
			return;
		}
		const bool bCleansed = Args.Num() < 1 || FCString::Atoi(*Args[0]) != 0 || Args[0].ToLower() == TEXT("true");
		Save->SetSaidinCleansed(bCleansed);
		UE_LOG(LogWotChannel, Display, TEXT("[PT] CHANNEL saidin cleansed=%d"), bCleansed ? 1 : 0);
		if (UWotChannelComponent* C = WotLocalChannel(World)) {
			if (AWotCharacter* Char = C->GetCharacter()) {
				Char->ShowPopupWidget(bCleansed ? NSLOCTEXT("Wot", "Cleansed", "saidin is clean") : NSLOCTEXT("Wot", "Tainted", "the taint lies on saidin"), 1.5f, true);
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GWotChannelState(
	TEXT("wot.Channel.State"), TEXT("Log the local channeler's state: half, cleansed, channeling, charge, chaos, ooze drops"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World) {
		UWotChannelComponent* C = WotLocalChannel(World);
		if (!C) {
			UE_LOG(LogWotChannel, Display, TEXT("[PT] CHANNEL no local channeler"));
			return;
		}
		UE_LOG(LogWotChannel, Display, TEXT("[PT] CHANNEL half=%s cleansed=%d tainted=%d channeling=%d charge=%.2f chaos=%.2f weave=%s"),
			C->Half == EWotPowerHalf::Saidar ? TEXT("saidar") : C->Half == EWotPowerHalf::Saidin ? TEXT("saidin") : TEXT("none"), UWotSaveSubsystem::IsSaidinCleansedIn(World) ? 1 : 0, C->IsTainted() ? 1 : 0,
			C->IsChanneling() ? 1 : 0, C->GetCharge(), C->GetChaos(), *UWotChannelComponent::WeaveName(C->SelectedWeave).ToString());
	}));
