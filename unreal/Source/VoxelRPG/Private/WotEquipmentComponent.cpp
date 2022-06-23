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
  // Attachment rules for the meshes to the sockets
  FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget,
                                            EAttachmentRule::SnapToTarget,
                                            EAttachmentRule::KeepWorld,
                                            true);
  // Create the components for the armor sockets
  for (auto& ArmorSocketName : ArmorSocketNames) {
    FString MeshName = ArmorSocketName.ToString() + FString("Mesh");
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Character, UStaticMeshComponent::StaticClass(), *MeshName);
    Mesh->SetCollisionProfileName("Item");
    // Set attachment point of owner
    Mesh->AttachToComponent(CharacterMesh, AttachmentRules, ArmorSocketName);
    Mesh->RegisterComponent();
    // now save the mesh pointer
    ArmorComponents.Add(ArmorSocketName, Mesh);
    UE_LOG(LogTemp, Warning, TEXT("Added Mesh for %s"), *ArmorSocketName.ToString());
  }
  // Create the components for the weapon sockets
  for (auto& WeaponSocketName : WeaponSocketNames) {
    FString MeshName = WeaponSocketName.ToString() + FString("Mesh");
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Character, UStaticMeshComponent::StaticClass(), *MeshName);
    Mesh->SetCollisionProfileName("Item");
    // Set attachment point of owner
    Mesh->AttachToComponent(CharacterMesh, AttachmentRules, WeaponSocketName);
    Mesh->RegisterComponent();
    // now save the mesh pointer
    WeaponComponents.Add(WeaponSocketName, Mesh);
    UE_LOG(LogTemp, Warning, TEXT("Added Mesh for %s"), *WeaponSocketName.ToString());
  }
  UE_LOG(LogTemp, Warning, TEXT("Initialized WotEquipmentComponent"));
}

void UWotEquipmentComponent::UnequipItem(UWotItemEquipment* NewItemEquipment) {
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

void UWotEquipmentComponent::EquipItem(UWotItemEquipment* NewItemEquipment) {
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
    // We have a weapon already equipped there, unequip it
    UnequipArmor(*EquippedArmor);
  }
  // Update the ArmorItems map
  ArmorItems.Add(SocketName, NewItemArmor);
  // set the item's equipped value to true
  NewItemArmor->IsEquipped = true;
  // Set the item mesh
  UStaticMeshComponent** MeshComponent = ArmorComponents.Find(SocketName);
  if (!MeshComponent) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot equip: Mesh Component is null!"));
    return;
  }
  if (NewItemArmor->PickupMesh) {
    (*MeshComponent)->SetStaticMesh(NewItemArmor->PickupMesh);
  }
}

void UWotEquipmentComponent::EquipWeapon(UWotItemWeapon* NewItemWeapon) {
  FName SocketName = NewItemWeapon->EquipSocketName;
  if (!WeaponSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot equip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  UWotItemWeapon** EquippedWeapon = WeaponItems.Find(SocketName);
  if (EquippedWeapon) {
    // We have a weapon already equipped there, unequip it
    UnequipWeapon(*EquippedWeapon);
  }
  // Update the WeaponItems map
  WeaponItems.Add(SocketName, NewItemWeapon);
  // set the item's equipped value to true
  NewItemWeapon->IsEquipped = true;
  // Set the item mesh
  UStaticMeshComponent** MeshComponent = WeaponComponents.Find(SocketName);
  if (!MeshComponent) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot equip: Mesh Component is null!"));
    return;
  }
  if (NewItemWeapon->PickupMesh) {
    (*MeshComponent)->SetStaticMesh(NewItemWeapon->PickupMesh);
  }
}

void UWotEquipmentComponent::UnequipArmor(UWotItemArmor* NewItemArmor) {
  FName SocketName = NewItemArmor->EquipSocketName;
  if (!ArmorSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  // set the item's equipped value to true
  NewItemArmor->IsEquipped = false;
  // remove the item from the map
  ArmorItems.Remove(SocketName);
  // Set the item mesh
  UStaticMeshComponent** MeshComponent = ArmorComponents.Find(SocketName);
  if (!MeshComponent) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip: Mesh Component is null!"));
    return;
  }
  (*MeshComponent)->SetStaticMesh(nullptr);
}

void UWotEquipmentComponent::UnequipWeapon(UWotItemWeapon* NewItemWeapon) {
  FName SocketName = NewItemWeapon->EquipSocketName;
  if (!WeaponSocketNames.Contains(SocketName)) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip weapon at unregistered socket %s"), *SocketName.ToString());
    return;
  }
  // set the item's equipped value to true
  NewItemWeapon->IsEquipped = false;
  // Update the WeaponItems map
  WeaponItems.Remove(SocketName);
  // Set the item mesh
  UStaticMeshComponent** MeshComponent = WeaponComponents.Find(SocketName);
  if (!MeshComponent) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot unequip: Mesh Component is null!"));
    return;
  }
  (*MeshComponent)->SetStaticMesh(nullptr);
}

UWotEquipmentComponent* UWotEquipmentComponent::GetEquipment(AActor* FromActor)
{
  if (FromActor) {
    return Cast<UWotEquipmentComponent>(FromActor->GetComponentByClass(UWotEquipmentComponent::StaticClass()));
  }
  return nullptr;
}
