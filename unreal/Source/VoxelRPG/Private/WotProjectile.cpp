// Fill out your copyright notice in the Description page of Project Settings.


#include "WotProjectile.h"
#include "WotAttributeComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AWotProjectile::AWotProjectile()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  SphereComp = CreateDefaultSubobject<USphereComponent>("SphereComp");
  // SphereComp->SetCollisionObjectType(ECC_WorldDynamic);
  // SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
  // SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  SphereComp->SetCollisionProfileName("Projectile");
  SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AWotProjectile::OnActorOverlap);
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

void AWotProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  if (OtherActor && OtherActor != GetInstigator()) {
    UWotAttributeComponent* AttributeComp = Cast<UWotAttributeComponent>(OtherActor->GetComponentByClass(UWotAttributeComponent::StaticClass()));
    if (AttributeComp) {
      AttributeComp->ApplyHealthChangeInstigator(GetInstigator(), Damage);
      Explode();
    }
  }
}

void AWotProjectile::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  SphereComp->IgnoreActorWhenMoving(GetInstigator(), true);
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
