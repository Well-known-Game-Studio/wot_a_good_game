// Fill out your copyright notice in the Description page of Project Settings.
#include "WotArrowProjectile.h"
#include "WotAttributeComponent.h"
#include "Items/WotItem.h"
#include "Items/WotItemInteractableActor.h"
#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/TriggerBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "WotGameplayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Math/UnrealMathUtility.h"

static TAutoConsoleVariable<bool> CVarDebugDrawArrowHits(TEXT("wot.DebugDrawArrowHits"), false, TEXT("Enable DebugDrawing for Arrow Projectile"), ECVF_Cheat);

AWotArrowProjectile::AWotArrowProjectile()
{
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

void AWotArrowProjectile::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  // The parent class enables collision in postinitialize, but we want to
  // disable collision until we're fired
  SphereComp->SetCollisionProfileName("NoCollision");
  SphereComp->OnComponentHit.AddDynamic(this, &AWotArrowProjectile::OnComponentHit);
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
      UE_LOG(LogTemp, Log, TEXT("On State Begin: Default"));
      break;
    }
    case EWotArrowState::InBow: {
      UE_LOG(LogTemp, Log, TEXT("On State Begin: InBow"));
      // Disable collision
      SphereComp->SetCollisionProfileName("NoCollision");
      break;
    }
    case EWotArrowState::InAir: {
      UE_LOG(LogTemp, Log, TEXT("On State Begin: InAir"));
      // standard projectile collision
      SphereComp->SetCollisionProfileName(CollisionProfileName);
      EffectAudioComp->SetSound(EffectSound);
      // TODO: offset for this specific sound
      EffectAudioComp->Play(0.2);
      if (IsValid(MovementComp)) {
        FVector ForwardVector = GetActorForwardVector();
        float Velocity = FMath::Lerp(MinVelocity, MaxVelocity, BowCharge);
        MovementComp->Velocity = ForwardVector * Velocity;
        MovementComp->Activate();
      }
      break;
    }
    case EWotArrowState::Unobtained: {
      UE_LOG(LogTemp, Log, TEXT("On State Begin: Unobtained"));
      // Don't need the shere component anymore
      SphereComp->SetCollisionProfileName("NoCollision");
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
      UE_LOG(LogTemp, Log, TEXT("On State End: Default"));
      break;
    }
    case EWotArrowState::InBow: {
      UE_LOG(LogTemp, Log, TEXT("On State End: InBow"));
      break;
    }
    case EWotArrowState::InAir: {
      UE_LOG(LogTemp, Log, TEXT("On State End: InAir"));
      MovementComp->Deactivate();
      break;
    }
    case EWotArrowState::Unobtained: {
      UE_LOG(LogTemp, Log, TEXT("On State End: Unobtained"));
      break;
    }
    default:
      break;
  }
}

void AWotArrowProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  UE_LOG(LogTemp, Log, TEXT("OnActorOverlap"));
  HandleCollision(OtherActor, SweepResult);
}

void AWotArrowProjectile::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
  UE_LOG(LogTemp, Log, TEXT("OnComponentHit"));
  HandleCollision(OtherActor, Hit);
}

bool AWotArrowProjectile::ShouldHitActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp) {
  // call the parent implementation first
  if (!Super::ShouldHitActor_Implementation(OtherActor, OtherComp)) {
    return false;
  }
  if (OtherActor == Shooter) {
    UE_LOG(LogTemp, Log, TEXT("HandleCollision: OtherActor == Shooter"));
    return false;
  }
  return true;
}

void AWotArrowProjectile::HandleCollision(AActor* OtherActor, const FHitResult& SweepResult)
{
  // We only want to handle collisions when we're in the air
  if (CurrentState != EWotArrowState::InAir) {
    return;
  }

  if (!ensure(ItemClass)) {
    UE_LOG(LogTemp, Error, TEXT("ArrowProjectile::ItemClass is null!"));
    // if we don't have an item class, then we can't create an item interactable, so we
    // just destroy the arrow
    Destroy();
    return;
  }

  // get the hit component from the hitresult
  UPrimitiveComponent* HitComponent = SweepResult.GetComponent();
  if (!ShouldHitActor(OtherActor, HitComponent)) {
    return;
  }

  if (HandleParry(OtherActor)) {
    // if we handled a parry, then we don't need to do anything else
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("%s Hit %s"), *GetNameSafe(this), *GetNameSafe(OtherActor));
  // set new state to unobtained
  SetArrowState(EWotArrowState::Unobtained);
  // Debug helping
	bool bDrawDebug = CVarDebugDrawArrowHits.GetValueOnGameThread();
  if (bDrawDebug) {
    UWotGameplayFunctionLibrary::DrawHitPointAndBounds(OtherActor, SweepResult);
  }
  // Where are we when we hit?
  FVector CurrentLocation = GetActorLocation();
  FRotator CurrentRotation = GetActorRotation();
  // play impact sound
  UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, CurrentLocation, 1.0f, 1.0f, 0.0f);
  // set the location
  FVector NewLocation = CurrentLocation + GetActorForwardVector() * PenetrationDepth;
  SetActorLocation(NewLocation, false, nullptr, ETeleportType::ResetPhysics);
  // Create WotItemInteractableActor (Actor in world)
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  // Spawn one actor for each item dropped
  UE_LOG(LogTemp, Log, TEXT("Spawning New ItemInteractable for arrow!"));
  AWotItemInteractableActor* NewItemInteractable =
    GetWorld()->SpawnActor<AWotItemInteractableActor>(AWotItemInteractableActor::StaticClass(),
                                                      NewLocation,
                                                      CurrentRotation,
                                                      SpawnParams);
  // if we have a projectile life span, then set it to destroy after that time
  if (ProjectileLifeSpan != 0.0f) {
    NewItemInteractable->SetLifeSpan(ProjectileLifeSpan);
  }
  // and create WotItem (for collecting into inventory)
  UE_LOG(LogTemp, Log, TEXT("Creating New Item!"));
  UWotItem* NewItem = NewObject<UWotItem>(NewItemInteractable, ItemClass);
  NewItem->OwningInventory = nullptr;
  NewItem->Count = 1;
  NewItem->World = GetWorld();
  NewItemInteractable->SetPhysicsAndCollision("Projectile", false, true);
  NewItemInteractable->SetItem(NewItem);
  // attach new item interactible to other (collided) actor
  FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld,
                                            EAttachmentRule::KeepWorld,
                                            EAttachmentRule::KeepWorld,
                                            true);
  NewItemInteractable->AttachToActor(OtherActor, AttachmentRules, FName());
  // if the actor is damage-able, then damage them
  UWotGameplayFunctionLibrary::ApplyDamage(Shooter, OtherActor, Damage + Damage * BowCharge);
  // Destroy this actor since we've now created the interactible item for it
  Destroy();
}

// _Implementation from it being marked as BlueprintNativeEvent
void AWotArrowProjectile::Explode_Implementation()
{
  UE_LOG(LogTemp, Log, TEXT("Arrows shouldn't explode.... should they?"));
}
