// Fill out your copyright notice in the Description page of Project Settings.


#include "WotMagicProjectile.h"
#include "WotAttributeComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"


AWotMagicProjectile::AWotMagicProjectile()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  SphereComp = CreateDefaultSubobject<USphereComponent>("SphereComp");
  /*
  SphereComp->SetCollisionObjectType(ECC_WorldDynamic);
  SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
  SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  */
  SphereComp->SetCollisionProfileName("Projectile");
  SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AWotMagicProjectile::OnActorOverlap);
  RootComponent = SphereComp;

  EffectComp = CreateDefaultSubobject<UParticleSystemComponent>("EffectComp");
  EffectComp->SetupAttachment(SphereComp);

  MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>("MovementComp");
  MovementComp->InitialSpeed = 1000.0f;
  MovementComp->bRotationFollowsVelocity = true;
  MovementComp->bInitialVelocityInLocalSpace = true;
}

void AWotMagicProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  if (OtherActor) {
    UWotAttributeComponent* AttributeComp = Cast<UWotAttributeComponent>(OtherActor->GetComponentByClass(UWotAttributeComponent::StaticClass()));
    if (AttributeComp) {
      AttributeComp->ApplyHealthChange(-20.0f);
      Destroy();
    }
  }
}


// Called when the game starts or when spawned
void AWotMagicProjectile::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotMagicProjectile::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
