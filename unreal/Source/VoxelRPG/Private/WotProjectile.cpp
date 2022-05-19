// Fill out your copyright notice in the Description page of Project Settings.


#include "WotProjectile.h"
#include "WotAttributeComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

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

  NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>("NiagaraComp");
  NiagaraComp->SetupAttachment(SphereComp);

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
      AttributeComp->ApplyHealthChange(-20.0f);
      Destroy();
    }
  }
}

// Called when the game starts or when spawned
void AWotProjectile::BeginPlay()
{
  Super::BeginPlay();
  if (NiagaraSystem) {
    NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, SphereComp, NAME_None, FVector(0.f), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, true);
    // Parameters can be set like this:
    // NiagaraComp->SetNiagaraVariableFloat(FString("StrengthCoef"), CoefStrength);
  }
  SphereComp->IgnoreActorWhenMoving(GetInstigator(), true);
  SetLifeSpan(1.0f);
}

// Called every frame
void AWotProjectile::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
