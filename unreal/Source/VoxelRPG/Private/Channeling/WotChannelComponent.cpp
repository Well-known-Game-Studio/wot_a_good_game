#include "Channeling/WotChannelComponent.h"
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
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogWotChannel, Log, All);

UWotChannelComponent::UWotChannelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	AttachEffect(ThreadsSystem, LeftHandSocket, true);
	AttachEffect(ThreadsSystem, RightHandSocket, true);
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
}

void UWotChannelComponent::ReleaseWeave()
{
	if (!bChanneling) {
		return;
	}
	const float Charge = 0.35f + 0.65f * GetCharge();
	bChanneling = false;
	StopEffects();
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
	if (GetWorld()) {
		GetWorld()->GetTimerManager().ClearTimer(ShieldTimer);
	}
	Super::EndPlay(EndPlayReason);
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
