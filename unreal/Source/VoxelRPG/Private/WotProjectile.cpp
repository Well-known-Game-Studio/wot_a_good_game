// Fill out your copyright notice in the Description page of Project Settings.


#include "WotProjectile.h"
#include "WotAttributeComponent.h"
#include "WotActionComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TriggerBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "WotGameplayFunctionLibrary.h"

AWotProjectile::AWotProjectile()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  SphereComp = CreateDefaultSubobject<USphereComponent>("SphereComp");
  RootComponent = SphereComp;

  StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComp");
  StaticMeshComp->SetupAttachment(SphereComp);

  EffectAudioComp = CreateDefaultSubobject<UAudioComponent>("EffectAudioComp");
  EffectAudioComp->SetupAttachment(SphereComp);
  EffectAudioComp->SetSound(EffectSound);

  EffectNiagaraComp = CreateDefaultSubobject<UNiagaraComponent>("EffectNiagaraComp");
  EffectNiagaraComp->SetupAttachment(SphereComp);

  MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>("MovementComp");
  MovementComp->InitialSpeed = 1000.0f;
  MovementComp->bRotationFollowsVelocity = true;
  MovementComp->bInitialVelocityInLocalSpace = true;
}

void AWotProjectile::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  if (bUseSphereForCollisionAndOverlap) {
    SphereComp->IgnoreActorWhenMoving(GetInstigator(), true);
    // SphereComp->SetCollisionObjectType(ECC_WorldDynamic);
    // SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    // SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereComp->SetCollisionProfileName(CollisionProfileName);
    SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AWotProjectile::OnActorOverlap);
  }
  if (EffectNiagaraSystem) {
    EffectNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(EffectNiagaraSystem, SphereComp, NAME_None, FVector(0.f), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, true);
  }

  SetLifeSpan(ProjectileLifeSpan);
}

// Called when the game starts or when spawned
void AWotProjectile::BeginPlay()
{
  Super::BeginPlay();
  EffectAudioComp->Play();
}


bool AWotProjectile::ShouldHitActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
  if (!OtherActor) {
    UE_LOG(LogTemp, Log, TEXT("HandleCollision: !OtherActor"));
    return false;
  }
  // TODO: this is a hack to ignore overlap with the fluid flux surfaces /
  // actors!
  if (GetNameSafe(OtherActor).Contains("flux")) {
    return false;
  }
  // if the other actor is a trigger box (or any other actor that we don't want to explode on)
  // then return
  if (OtherActor->IsA(ATriggerBase::StaticClass())) {
    UE_LOG(LogTemp, Log, TEXT("HandleCollision: OtherActor->IsA(ATriggerBase::StaticClass())"));
    return false;
  }
  if (OtherActor == this) {
    UE_LOG(LogTemp, Log, TEXT("Projectiles shouldn't be able to collide with themselves... should they?"));
    return false;
  }
  if (OtherActor == GetInstigator()) {
    UE_LOG(LogTemp, Log, TEXT("HandleCollision: OtherActor == GetInstigator()"));
    return false;
  }
  return true;
}

void AWotProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  if (!ShouldHitActor(OtherActor, OtherComp)) {
    return;
  }

  // print the display name of the actor we overlapped with
  UE_LOG(LogTemp, Warning, TEXT("Overlapped with %s"), *OtherActor->GetName());

  UWotActionComponent* ActionComp = UWotActionComponent::GetActions(OtherActor);
  if (ActionComp && ActionComp->ActiveGameplayTags.HasTag(ParryTag)) {
    UE_LOG(LogTemp, Log, TEXT("Projectile was parried!"));
    // reflect projectile back to where it came from
    MovementComp->Velocity = -MovementComp->Velocity;
    // Make sure to update the instigator so that it can damage the original actor if it hits them
    SetInstigator(Cast<APawn>(OtherActor));
    // return here so we don't explode or try to apply damage
    return;
  }

  UWotGameplayFunctionLibrary::ApplyDirectionalDamage(GetInstigator(), OtherActor, Damage, SweepResult);
  Explode();
}

// _Implementation from it being marked as BlueprintNativeEvent
void AWotProjectile::Explode_Implementation()
{
  // Check to make sure we aren't already being 'destroyed'
  // Adding ensure to see if we encounter this situation at all
  if (ensure(IsValid(this))) {
    if (ImpactNiagaraSystem) {
      auto ImpactNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,
                                                                              ImpactNiagaraSystem,
                                                                              GetActorLocation(),
                                                                              GetActorRotation());
    }
    EffectNiagaraComp->Deactivate();
    MovementComp->StopMovementImmediately();
    SetActorEnableCollision(false);
    if (ImpactSound) {
      UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), 1.0f, 1.0f, 0.0f);
    }
    if (CameraShakeEffect) {
      UGameplayStatics::PlayWorldCameraShake(this,
                                             CameraShakeEffect,
                                             GetActorLocation(),
                                             CameraShakeInnerRadius,
                                             CameraShakeOuterRadius,
                                             CameraShakeFalloff);
    }
    Destroy();
  }
}

// Called every frame
void AWotProjectile::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
