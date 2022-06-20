// Fill out your copyright notice in the Description page of Project Settings.

#include "WotWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"
#include "Items/WotItemWeapon.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
UWotWeaponComponent::UWotWeaponComponent()
{
  Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
  // Set physics enabled on this actor
  Mesh->SetCollisionProfileName("Item");
}

void UWotWeaponComponent::SetItem(UWotItemWeapon* NewItemWeapon) {
  ItemWeapon = NewItemWeapon;
  // Set the item mesh
  if (ItemWeapon->PickupMesh) {
    Mesh->SetStaticMesh(ItemWeapon->PickupMesh);
  }
  // Now attach it to the character's skeletal mesh
  ACharacter* Character = Cast<ACharacter>(GetOwner());
  if (!ensure(Character)) {
    UE_LOG(LogTemp, Warning, TEXT("No Character!"));
    return;
  }
  USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
  if (!ensure(CharacterMesh)) {
    UE_LOG(LogTemp, Warning, TEXT("No Mesh!"));
    return;
  }
  if (ItemWeapon->EquipSocketName.IsNone()) {
    UE_LOG(LogTemp, Warning, TEXT("No Equip socket name!"));
    return;
  }
  // Set attachment point of owner
  FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
  Mesh->AttachToComponent(CharacterMesh, AttachmentRules, ItemWeapon->EquipSocketName);
}

bool UWotWeaponComponent::PrimaryAttack()
{
  return true;
}

bool UWotWeaponComponent::SecondaryAttack()
{
  return true;
}

UWotWeaponComponent* UWotWeaponComponent::GetWeapon(AActor* FromActor)
{
  if (FromActor) {
    return Cast<UWotWeaponComponent>(FromActor->GetComponentByClass(UWotWeaponComponent::StaticClass()));
  }
  return nullptr;
}
