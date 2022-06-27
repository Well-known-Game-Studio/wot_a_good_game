// Fill out your copyright notice in the Description page of Project Settings.
#include "Items/WotEquippedWeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Items/WotItemWeapon.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"

// Sets default values
AWotEquippedWeaponActor::AWotEquippedWeaponActor() : AWotItemActor()
{
  SetPhysicsAndCollision("Item", false, false);
}

void AWotEquippedWeaponActor::PrimaryAttackStart_Implementation()
{
}

void AWotEquippedWeaponActor::PrimaryAttackStop_Implementation()
{
}

void AWotEquippedWeaponActor::SecondaryAttackStart_Implementation()
{
}

void AWotEquippedWeaponActor::SecondaryAttackStop_Implementation()
{
}
