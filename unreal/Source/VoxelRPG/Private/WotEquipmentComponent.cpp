// Fill out your copyright notice in the Description page of Project Settings.

#include "WotEquipmentComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"
#include "Items/WotItemEquipment.h"
#include "Items/WotItemArmor.h"
#include "Items/WotItemWeapon.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"

// Sets default values
UWotEquipmentComponent::UWotEquipmentComponent()
{
  bWantsInitializeComponent = true;
}

void UWotEquipmentComponent::InitializeComponent()
{
  Super::InitializeComponent();
}

void UWotEquipmentComponent::UnequipItem(UWotItem* NewItem) {
  UE_LOG(LogTemp, Log, TEXT("Unequipping Item %s"), *GetNameSafe(NewItem));
 UWotItemEquipment* NewItemEquipment = Cast<UWotItemEquipment>(NewItem);
  if (!NewItemEquipment) {
    UE_LOG(LogTemp, Warning, TEXT("Not a valid ItemEquipment!"));
    return;
  }
  if (!NewItemEquipment->CanBeEquipped) {
    UE_LOG(LogTemp, Warning, TEXT("Item cannot be equipped, not unequipping!"));
    return;
  }
  if (NewItemEquipment->EquipSocketName.IsNone()) {
    UE_LOG(LogTemp, Warning, TEXT("No Equip socket name to unequip from!"));
    return;
  }
  UWotItemWeapon* NewItemWeapon = Cast<UWotItemWeapon>(NewItemEquipment);
  UWotItemArmor* NewItemArmor = Cast<UWotItemArmor>(NewItemEquipment);
  if (NewItemWeapon) {
    UnequipWeapon(NewItemWeapon);
  } else if (NewItemArmor) {
    UnequipArmor(NewItemArmor);
  } else {
    UE_LOG(LogTemp, Warning, TEXT("Item cannot be unequipped, it is not a subclass of either Armor or Weapon"));
  }
}

void UWotEquipmentComponent::EquipItem(UWotItem* NewItem) {
  UE_LOG(LogTemp, Log, TEXT("Equipping Item %s"), *GetNameSafe(NewItem));
  UWotItemEquipment* NewItemEquipment = Cast<UWotItemEquipment>(NewItem);
  if (!NewItemEquipment) {
    UE_LOG(LogTemp, Warning, TEXT("Not a valid ItemEquipment!"));
    return;
  }
  if (!NewItemEquipment->CanBeEquipped) {
    UE_LOG(LogTemp, Warning, TEXT("Item cannot be equipped, not equipping!"));
    return;
  }
  if (NewItemEquipment->EquipSocketName.IsNone()) {
    UE_LOG(LogTemp, Warning, TEXT("No Equip socket name!"));
    return;
  }
  UWotItemWeapon* NewItemWeapon = Cast<UWotItemWeapon>(NewItemEquipment);
  UWotItemArmor* NewItemArmor = Cast<UWotItemArmor>(NewItemEquipment);
  if (NewItemWeapon) {
    EquipWeapon(NewItemWeapon);
  } else if (NewItemArmor) {
    EquipArmor(NewItemArmor);
  } else {
    UE_LOG(LogTemp, Warning, TEXT("Item cannot be equipped, it is not a subclass of either Armor or Weapon"));
  }
}

void UWotEquipmentComponent::EquipArmor(UWotItemArmor* NewItemArmor) {
  FName SocketName = NewItemArmor->EquipSocketName;
  if (!ArmorSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot equip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  UWotItemArmor** EquippedArmor = ArmorItems.Find(SocketName);
  if (EquippedArmor) {
    UE_LOG(LogTemp, Warning, TEXT("Equip: already equipped, unequipping previously equipped armor!"));
    // We have a weapon already equipped there, unequip it
    UnequipArmor(*EquippedArmor);
  }
  // Update the ArmorItems map
  ArmorItems.Add(SocketName, NewItemArmor);
  // Let the item know it's equipped and have it update rendering accordingly
  NewItemArmor->Equip(Cast<ACharacter>(GetOwner()));
}

void UWotEquipmentComponent::EquipWeapon(UWotItemWeapon* NewItemWeapon) {
  FName SocketName = NewItemWeapon->EquipSocketName;
  if (!WeaponSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot equip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  UWotItemWeapon** EquippedWeapon = WeaponItems.Find(SocketName);
  if (EquippedWeapon) {
    UE_LOG(LogTemp, Warning, TEXT("Equip: already equipped, unequipping previously equipped weapon!"));
    // We have a weapon already equipped there, unequip it
    UnequipWeapon(*EquippedWeapon);
  }
  // Update the WeaponItems map
  WeaponItems.Add(SocketName, NewItemWeapon);
  // Let the item know it's equipped and have it update rendering accordingly
  NewItemWeapon->Equip(Cast<ACharacter>(GetOwner()));
}

void UWotEquipmentComponent::UnequipAll() {
  for (auto& Elem : ArmorItems) {
    auto ArmorItem = Elem.Value;
    if (ArmorItem) {
      ArmorItem->Unequip(Cast<ACharacter>(GetOwner()));
    }
  }
  for (auto& Elem : WeaponItems) {
    auto WeaponItem = Elem.Value;
    if (WeaponItem) {
      WeaponItem->Unequip(Cast<ACharacter>(GetOwner()));
    }
  }
}

void UWotEquipmentComponent::UnequipArmor(UWotItemArmor* NewItemArmor) {
  FName SocketName = NewItemArmor->EquipSocketName;
  if (!ArmorSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  // Let the item know it's unequipped and update rendering accordingly
  NewItemArmor->Unequip(Cast<ACharacter>(GetOwner()));
  // remove the item from the map
  ArmorItems.Remove(SocketName);
}

void UWotEquipmentComponent::UnequipWeapon(UWotItemWeapon* NewItemWeapon) {
  FName SocketName = NewItemWeapon->EquipSocketName;
  if (!WeaponSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  // Let the item know it's unequipped and update rendering accordingly
  NewItemWeapon->Unequip(Cast<ACharacter>(GetOwner()));
  // Update the WeaponItems map
  WeaponItems.Remove(SocketName);
}

UWotItemWeapon* UWotEquipmentComponent::GetEquippedWeapon()
{
  if (WeaponItems.Num()) {
    auto It = WeaponItems.CreateConstIterator();
    return It->Value;
  }
  UE_LOG(LogTemp, Log, TEXT("No weapon equipped!"));
  return nullptr;
}

UWotEquipmentComponent* UWotEquipmentComponent::GetEquipment(AActor* FromActor)
{
  if (FromActor) {
    return Cast<UWotEquipmentComponent>(FromActor->GetComponentByClass(UWotEquipmentComponent::StaticClass()));
  }
  return nullptr;
}

UWotItemWeapon* UWotEquipmentComponent::GetEquippedWeaponFromActor(AActor* FromActor)
{
  UWotEquipmentComponent* EquipmentComponent = GetEquipment(FromActor);
  if (!EquipmentComponent) {
    return nullptr;
  }
  return EquipmentComponent->GetEquippedWeapon();
}
