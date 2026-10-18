#include "Channeling/WotTiedWeave.h"
#include "WotAttributeComponent.h"
#include "WotGameplayFunctionLibrary.h"
#include "AI/WotAICharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

AWotTiedWeave::AWotTiedWeave()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Reach = CreateDefaultSubobject<USphereComponent>(TEXT("Reach"));
	Reach->SetupAttachment(Root);
	Reach->SetSphereRadius(Radius);
	Reach->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Reach->SetCollisionResponseToAllChannels(ECR_Ignore);
	Reach->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Wall = CreateDefaultSubobject<UBoxComponent>(TEXT("Wall"));
	Wall->SetupAttachment(Root);
	Wall->SetBoxExtent(FVector(22.0f, 170.0f, 115.0f));
	Wall->SetRelativeLocation(FVector(0, 0, 115.0f));
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wall->SetCollisionProfileName(TEXT("BlockAll"));
	Wall->SetCanEverAffectNavigation(false);
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0, 0, 110.0f));
	Light->SetAttenuationRadius(700.0f);
	Light->SetCastShadows(false);
	Light->SetIntensity(2500.0f);
	ThreadSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Wot/Effects/NS_WeaveThread.NS_WeaveThread")));
	TaintSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Ultimate_Magic_Bundle/MagicProjectiles_Optimal/Niagara/Shadow/NS_Shadow_Trail_Purple.NS_Shadow_Trail_Purple")));
}

void AWotTiedWeave::Configure(EWotWeave InWeave, EWotPowerHalf InHalf, float InPower, const FLinearColor& InColor, float InDuration, bool bInTainted)
{
	Weave = InWeave;
	Half = InHalf;
	bTainted = bInTainted && InHalf == EWotPowerHalf::Saidin;
	Power = FMath::Max(0.1f, InPower);
	Color = InColor;
	Duration = InDuration;
	switch (Weave) {
	case EWotWeave::Fire: Radius = 240.0f + 80.0f * Power; break;
	case EWotWeave::Air: Radius = 300.0f + 120.0f * Power; break;
	case EWotWeave::Water: Radius = 280.0f + 60.0f * Power; break;
	case EWotWeave::Earth: Radius = 220.0f; break;
	}
}

void AWotTiedWeave::BeginPlay()
{
	Super::BeginPlay();
	Reach->SetSphereRadius(Radius);
	Light->SetLightColor(Color);
	Light->SetIntensity(2200.0f * FMath::Sqrt(Power));
	if (Weave == EWotWeave::Earth) {
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetBoxExtent(FVector(22.0f, 150.0f + 40.0f * Power, 115.0f));
	}
	BuildKnot();
	if (bTainted) {
		// shadow seeps from the heart of the knot
		if (UNiagaraSystem* Taint = TaintSystem.LoadSynchronous()) {
			if (UNiagaraComponent* T = UNiagaraFunctionLibrary::SpawnSystemAttached(Taint, Root, NAME_None, FVector(0, 0, Weave == EWotWeave::Earth ? 120.0f : 100.0f + 30.0f * Power), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true)) {
				Threads.Add(T);
				ThreadAmplitudes.Add(-1.0f);   // not a ribbon
			}
		}
	}
}

UNiagaraComponent* AWotTiedWeave::SpawnThread(const FVector& From, const FVector& To, float Phase, float Amplitude, float Width)
{
	UNiagaraSystem* System = ThreadSystem.LoadSynchronous();
	if (!System || !GetWorld()) {
		return nullptr;
	}
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, From, FRotator::ZeroRotator, FVector(1.0f), false, true, ENCPoolMethod::None, true);
	if (!Comp) {
		return nullptr;
	}
	Comp->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
	Comp->SetVariableVec3(FName("User.Source"), From);
	Comp->SetVariableVec3(FName("User.Target"), To);
	Comp->SetVariableLinearColor(FName("User.Color"), Color);
	Comp->SetVariableFloat(FName("User.Width"), Width);
	Comp->SetVariableFloat(FName("User.Phase"), Phase);
	Comp->SetVariableFloat(FName("User.Amplitude"), Amplitude);
	Comp->SetVariableFloat(FName("User.Gather"), 1.0f);
	Threads.Add(Comp);
	ThreadAmplitudes.Add(Amplitude);
	return Comp;
}

void AWotTiedWeave::BuildKnot()
{
	const FVector C = GetActorLocation();
	FRandomStream Rand(GetUniqueID());
	if (Weave == EWotWeave::Earth) {
		// a lattice across the wall: threads from the bottom corners to the top corners and across
		const FVector Right = GetActorRightVector();
		const float HalfW = Wall->GetUnscaledBoxExtent().Y - 10.0f;
		const float H = Wall->GetUnscaledBoxExtent().Z * 2.0f;
		for (int32 i = 0; i < KnotThreads; ++i) {
			const float A = -HalfW + (2.0f * HalfW) * (float(i) / FMath::Max(1, KnotThreads - 1));
			const float B = HalfW - (2.0f * HalfW) * (float(i) / FMath::Max(1, KnotThreads - 1));
			SpawnThread(C + Right * A + FVector(0, 0, 4.0f), C + Right * B + FVector(0, 0, H), Rand.FRand() * 6.28f, 16.0f, 18.0f);
		}
		return;
	}
	// the knot: threads from the ground ring up into a braided centre, plus a few arcs between the anchors
	const FVector Centre = C + FVector(0, 0, 120.0f + 30.0f * Power);
	TArray<FVector> Anchors;
	for (int32 i = 0; i < KnotThreads; ++i) {
		const float Ang = (float(i) / KnotThreads) * 2.0f * PI + Rand.FRand() * 0.4f;
		const float R = Radius * (0.5f + 0.3f * Rand.FRand());
		FVector A = C + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 3.0f);
		Anchors.Add(A);
		SpawnThread(A, Centre, Rand.FRand() * 6.28f, 26.0f + 12.0f * Power, 18.0f + 6.0f * Power);
	}
	for (int32 i = 0; i < Anchors.Num(); i += 2) {
		const FVector& A = Anchors[i];
		const FVector& B = Anchors[(i + 3) % Anchors.Num()];
		SpawnThread(A, B, Rand.FRand() * 6.28f, 45.0f, 12.0f);
	}
}

void AWotTiedWeave::Unravel()
{
	if (bUnravelling) {
		return;
	}
	bUnravelling = true;
	for (UNiagaraComponent* T : Threads) {
		if (T) {
			T->SetVariableFloat(FName("User.Width"), 1.0f);
			T->SetVariableFloat(FName("User.Amplitude"), 90.0f);
		}
	}
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(0.6f);
}

void AWotTiedWeave::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;
	if (bUnravelling) {
		Light->SetIntensity(FMath::Max(0.0f, Light->Intensity - 5000.0f * DeltaTime));
		return;
	}
	if (Age >= Duration) {
		Unravel();
		return;
	}
	// breathing light, brighter as the knot nears its end
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Age * 5.0f);
	Light->SetIntensity(2200.0f * FMath::Sqrt(Power) * Pulse);
	if (Half == EWotPowerHalf::Saidin) {
		UpdateSaidinLook();
	}

	const FVector C = GetActorLocation();
	APawn* Channeler = GetInstigator();
	PulseAccumulator += DeltaTime;
	switch (Weave) {
	case EWotWeave::Fire:
		if (PulseAccumulator >= 0.5f) {
			PulseAccumulator = 0.0f;
			for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
				AWotAICharacter* T = *It;
				const FVector To = T->GetActorLocation() - C;
				if (To.Size() > Radius) {
					continue;
				}
				FHitResult Fake;
				Fake.ImpactPoint = T->GetActorLocation();
				Fake.ImpactNormal = -To.GetSafeNormal();
				Fake.Location = Fake.ImpactPoint;
				Fake.TraceStart = C;
				Fake.TraceEnd = T->GetActorLocation();
				UWotGameplayFunctionLibrary::ApplyDirectionalDamage(Channeler ? static_cast<AActor*>(Channeler) : this, T, -FireDamagePerSecond * 0.5f * Power, Fake, 250.0f);
			}
		}
		break;
	case EWotWeave::Air:
		for (TActorIterator<AWotAICharacter> It(GetWorld()); It; ++It) {
			AWotAICharacter* T = *It;
			const FVector To = T->GetActorLocation() - C;
			if (To.Size() > Radius) {
				continue;
			}
			if (UCharacterMovementComponent* Move = T->GetCharacterMovement()) {
				const float Falloff = 1.0f - FMath::Clamp(To.Size() / Radius, 0.0f, 0.9f);
				Move->AddImpulse(To.GetSafeNormal2D() * AirPush * Power * Falloff * DeltaTime * 4.0f + FVector(0, 0, 60.0f * DeltaTime * 4.0f), true);
			}
		}
		break;
	case EWotWeave::Water:
		if (PulseAccumulator >= 1.0f) {
			PulseAccumulator = 0.0f;
			for (TActorIterator<ACharacter> It(GetWorld()); It; ++It) {
				ACharacter* T = *It;
				if (Cast<AWotAICharacter>(T) || FVector::Dist(T->GetActorLocation(), C) > Radius) {
					continue;
				}
				if (UWotAttributeComponent* Attr = UWotAttributeComponent::GetAttributes(T)) {
					Attr->ApplyHealthChange(WaterHealPerSecond * Power);
				}
			}
		}
		break;
	case EWotWeave::Earth:
		break;
	}
}

void AWotTiedWeave::UpdateSaidinLook()
{
	// a saidin knot never rests: the ribbons thrash; the taint drowns them in flickering black and the light gutters,
	// clean saidin flares cold white now and then
	const float N = FMath::PerlinNoise1D(Age * (bTainted ? 3.0f : 6.0f) + GetUniqueID() * 0.37f);
	const float Surge = FMath::PerlinNoise1D(Age * 1.1f + 400.0f) > 0.62f ? 1.0f : 0.0f;
	const FLinearColor Dark(0.03f, 0.0f, 0.05f);
	for (int32 i = 0; i < Threads.Num(); ++i) {
		UNiagaraComponent* T = Threads[i];
		const float Rest = ThreadAmplitudes.IsValidIndex(i) ? ThreadAmplitudes[i] : -1.0f;
		if (!T || Rest < 0.0f) {
			continue;
		}
		const float Ni = FMath::PerlinNoise1D(Age * 5.0f + i * 19.0f);
		T->SetVariableFloat(FName("User.Amplitude"), Rest * (1.0f + (bTainted ? 0.7f : 1.1f) * FMath::Abs(Ni) + Surge * 0.8f));
		if (bTainted) {
			T->SetVariableLinearColor(FName("User.Color"), FMath::Lerp(Color, Dark, 0.3f + 0.5f * FMath::Clamp(0.5f + 0.5f * Ni, 0.0f, 1.0f) + Surge * 0.2f));
		} else if (Surge > 0.0f) {
			T->SetVariableLinearColor(FName("User.Color"), FMath::Lerp(Color, FLinearColor(1.0f, 1.0f, 1.0f) * 2.0f, 0.6f));
		} else {
			T->SetVariableLinearColor(FName("User.Color"), Color);
		}
	}
	if (bTainted) {
		Light->SetLightColor(FMath::Lerp(Color, FLinearColor(0.35f, 0.05f, 0.5f), 0.5f));
		Light->SetIntensity(Light->Intensity * (0.4f + 0.6f * FMath::Abs(N)));
	} else {
		Light->SetIntensity(Light->Intensity * (0.8f + 0.5f * FMath::Abs(N) + Surge * 0.8f));
	}
}
