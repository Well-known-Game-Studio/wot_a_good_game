// Fill out your copyright notice in the Description page of Project Settings.
#include "WotArrowProjectile.h"
#include "WotAttributeComponent.h"
#include "Items/WotItem.h"
#include "Items/WotItemInteractibleActor.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Math/UnrealMathUtility.h"

AWotArrowProjectile::AWotArrowProjectile()
{
  bUseSphereForCollisionAndOverlap = false;
  ProjectileLifeSpan = 0.0f;
  CurrentState = EWotArrowState::Default;
}

void AWotArrowProjectile::BeginPlay()
{
  Super::BeginPlay();
  // The parent class starts the effect audio comp in its BeginPlay, but we
  // don't want the effect audio to play until we fire
  EffectAudioComp->Stop();
}

void AWotArrowProjectile::Fire(AActor* NewShooter, float NewBowCharge)
{
  Shooter = NewShooter;
  BowCharge = NewBowCharge;
  SetArrowState(EWotArrowState::InAir);
}

void AWotArrowProjectile::SetArrowState(EWotArrowState NewArrowState)
{
  if (NewArrowState == CurrentState) {
    return;
  }
  OnStateEnd(CurrentState);
  CurrentState = NewArrowState;
  OnStateBegin(CurrentState);
}

void AWotArrowProjectile::OnStateBegin(EWotArrowState BeginArrowState)
{
  switch (BeginArrowState) {
    case EWotArrowState::Default: {
      break;
    }
    case EWotArrowState::InBow: {
      StaticMeshComp->SetCollisionProfileName("NoCollision", true);
      break;
    }
    case EWotArrowState::InAir: {
      StaticMeshComp->SetCollisionProfileName("OverlapAll", true);
      EffectAudioComp->Play(0);
      if (IsValid(MovementComp)) {
        FVector ForwardVector = GetActorForwardVector();
        float Velocity = FMath::Lerp(MinVelocity, MaxVelocity, BowCharge);
        MovementComp->Velocity = ForwardVector * Velocity;
        MovementComp->Activate();
      }
      break;
    }
    case EWotArrowState::Unobtained: {
      StaticMeshComp->SetCollisionProfileName("OverlapAllDynamic", true);
      EffectAudioComp->Stop();
      break;
    }
    default:
      break;
  }
}

void AWotArrowProjectile::OnStateEnd(EWotArrowState EndArrowState)
{
  switch (EndArrowState) {
    case EWotArrowState::Default: {
      break;
    }
    case EWotArrowState::InBow: {
      break;
    }
    case EWotArrowState::InAir: {
      MovementComp->Deactivate();
      break;
    }
    case EWotArrowState::Unobtained: {
      break;
    }
    default:
      break;
  }
}

void AWotArrowProjectile::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  HandleCollision(OtherActor, SweepResult);
}

void AWotArrowProjectile::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
  HandleCollision(OtherActor, Hit);
}

void AWotArrowProjectile::HandleCollision(AActor* OtherActor, const FHitResult& SweepResult)
{
  if (!OtherActor) {
    return;
  }
  if (OtherActor == GetInstigator()) {
    return;
  }
  if (OtherActor == Shooter) {
    return;
  }
  if (!ensure(ItemClass)) {
    UE_LOG(LogTemp, Error, TEXT("ArrowProjectile::ItemClass is null!"));
    return;
  }

  switch (CurrentState) {
    case EWotArrowState::Default: {
      break;
    }
    case EWotArrowState::InBow: {
      break;
    }
    case EWotArrowState::InAir: {
      FVector CurrentLocation = GetActorLocation();
      FRotator CurrentRotation = GetActorRotation();
      // set new state to unobtained
      SetArrowState(EWotArrowState::Unobtained);
      // play impact sound
      UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, CurrentLocation, 1.0f, 1.0f, 0.0f);
      // set the location
      FVector NewLocation = CurrentLocation + GetActorForwardVector() * PenetrationDepth;
      SetActorLocation(NewLocation, false, nullptr, ETeleportType::ResetPhysics);
      // Create WotItemInteractibleActor (Actor in world)
      FActorSpawnParameters SpawnParams;
      SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
      // Spawn one actor for each item dropped
      AWotItemInteractibleActor* NewItemInteractible =
        GetWorld()->SpawnActor<AWotItemInteractibleActor>(AWotItemInteractibleActor::StaticClass(),
                                                          NewLocation,
                                                          CurrentRotation,
                                                          SpawnParams);
      // and create WotItem (for collecting into inventory)
      UWotItem* NewItem = NewObject<UWotItem>(NewItemInteractible, ItemClass);
      NewItem->OwningInventory = nullptr;
      NewItem->Count = 1;
      NewItemInteractible->SetPhysicsAndCollision("Projectile", false, true);
      NewItemInteractible->SetItem(NewItem);
      // attach new item interactible to other (collided) actor
      FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld,
                                                EAttachmentRule::KeepWorld,
                                                EAttachmentRule::KeepWorld,
                                                true);
      NewItemInteractible->AttachToActor(OtherActor, AttachmentRules, FName());
      // if the actor is damage-able, then damage them
      UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(OtherActor);
      if (AttributeComp) {
        AttributeComp->ApplyHealthChangeInstigator(Shooter, Damage + Damage * BowCharge);
      }
      // Destroy this actor since we've now created the interactible item for it
      Destroy();
      break;
    }
    case EWotArrowState::Unobtained: {
      break;
    }
    default:
      break;
  }
}

void AWotArrowProjectile::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  StaticMeshComp->SetCollisionProfileName(CollisionProfileName);
  StaticMeshComp->OnComponentBeginOverlap.AddDynamic(this, &AWotArrowProjectile::OnComponentBeginOverlap);
  StaticMeshComp->OnComponentHit.AddDynamic(this, &AWotArrowProjectile::OnComponentHit);
  StaticMeshComp->IgnoreActorWhenMoving(GetInstigator(), true);

  EffectAudioComp->SetSound(EffectSound);
}

// _Implementation from it being marked as BlueprintNativeEvent
void AWotArrowProjectile::Explode_Implementation()
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