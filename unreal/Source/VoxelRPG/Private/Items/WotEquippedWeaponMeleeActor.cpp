// Fill out your copyright notice in the Description page of Project Settings.
#include "Items/WotEquippedWeaponMeleeActor.h"
#include "Items/WotItemWeapon.h"
#include "WotAttributeComponent.h"
#include "WotCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"

static TAutoConsoleVariable<bool> CVarDebugDrawInteraction(TEXT("wot.DebugDrawMeleeHitBox"), false, TEXT("Enable DebugDrawing for Equipped Weapon Melee Actor"), ECVF_Cheat);

// Sets default values
AWotEquippedWeaponMeleeActor::AWotEquippedWeaponMeleeActor() : AWotEquippedWeaponActor()
{
}

void AWotEquippedWeaponMeleeActor::PrimaryAttackStart_Implementation()
{
	AActor* MyOwner = GetAttachParentActor();
  ACharacter* MyCharacter = Cast<ACharacter>(MyOwner);
  if (!MyCharacter) {
    UE_LOG(LogTemp, Warning, TEXT("Not a valid Character!"));
    return;
  }
  // Get AnimInstance
  UWotCharacterAnimInstance* AnimInstance = Cast<UWotCharacterAnimInstance>(MyCharacter->GetMesh()->GetAnimInstance());
  if (!AnimInstance) {
    UE_LOG(LogTemp, Warning, TEXT("Not a valid WotCharacterAnimInstance!"));
    return;
  }
  // Try to attack
  bool DidAttack = AnimInstance->LightAttack();
  if (!DidAttack) {
    UE_LOG(LogTemp, Warning, TEXT("Could not attack!"));
    return;
  }

  // Now perform the sweep
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

  auto OwnerLocation = MyOwner->GetActorLocation();
	auto ForwardVector = MyOwner->GetActorForwardVector();
  auto OwnerRotation = MyOwner->GetActorRotation();

	FVector End = OwnerLocation + (ForwardVector * AttackRange);

	TArray<FHitResult> Hits;

	FCollisionShape Shape;
	Chaos::TVector<float, 3> HalfExtent = HitBoxHalfExtent;
    FVector Extent = FVector(HalfExtent.X, HalfExtent.Y, HalfExtent.Z);
	Shape.SetBox(HalfExtent);

	bool bBlockingHit = GetWorld()->SweepMultiByObjectType(Hits,
                                                         OwnerLocation,
                                                         End,
                                                         OwnerRotation.Quaternion(),
                                                         ObjectQueryParams,
                                                         Shape);

	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

  // Get damage to use
  UWotItemWeapon* ItemWeapon = Cast<UWotItemWeapon>(Item);
  if (!ItemWeapon) {
    UE_LOG(LogTemp, Warning, TEXT("Not a valid weapon item!"));
    return;
  }
  float Damage = -ItemWeapon->DamageAmount;

	for (auto Hit : Hits) {
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor != MyOwner) {
      // if the actor is damage-able, then damage them
      UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(HitActor);
      if (AttributeComp) {
        AttributeComp->ApplyHealthChangeInstigator(MyOwner, Damage);
				if (bDrawDebug) {
          FVector HitActorLocation;
          FVector HitBoxExtent;
          HitActor->GetActorBounds(false, HitActorLocation, HitBoxExtent, false);
          // draw a box around what was hit
					DrawDebugBox(GetWorld(), HitActorLocation, HitBoxExtent, HitActor->GetActorRotation().Quaternion(), FColor::Green, false, 2.0f, 0, 2.0f);
          // draw a point for the hit location itself
          DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10, FColor::Red, false, 2.0f, 100);
				}
      }
		}
	}

	if (bDrawDebug) {
		FColor LineColor = bBlockingHit ? FColor::Green : FColor::Red;
    // Draw a line for the vector from the owner to the end of the sweep
		DrawDebugLine(GetWorld(), OwnerLocation, End, LineColor, false, 5.0f, 0, 10.0f);
    // Draw the box representing how we swept
    FVector SweepExtent(AttackRange / 2, HalfExtent.Y, HalfExtent.Z);
    DrawDebugBox(GetWorld(),
                 OwnerLocation + (ForwardVector * AttackRange) / 2,
                 SweepExtent,
                 OwnerRotation.Quaternion(),
                 LineColor, false, 2.0f, 0, 2.0f);
	}
}

void AWotEquippedWeaponMeleeActor::PrimaryAttackStop_Implementation()
{
}

void AWotEquippedWeaponMeleeActor::SecondaryAttackStart_Implementation()
{
}

void AWotEquippedWeaponMeleeActor::SecondaryAttackStop_Implementation()
{
}
