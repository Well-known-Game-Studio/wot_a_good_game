#include "Items/WotItemEquipment.h"
#include "Items/WotItemActor.h"
#include "GameFramework/Character.h"
#include "WotEquipmentComponent.h"

UWotItemEquipment::UWotItemEquipment() : UWotItem()
{
  UseActionText = FText::FromString("Equip");
  Count = 1;
  // TODO: do we want a max count of one for equipment?
  MaxCount = 1;
  // TODO: Handle multiple possible socket names (e.g. left/right hands)
  EquipSocketName = "Hand_R";
  IsEquipped = false;
}

void UWotItemEquipment::Copy(const UWotItem* OtherItem) {
  Super::Copy(OtherItem);
  // Specialization
  const UWotItemEquipment* OtherEquipment = Cast<UWotItemEquipment>(OtherItem);
  if (OtherEquipment) {
    EquipSocketName = OtherEquipment->EquipSocketName;
    IsEquipped = OtherEquipment->IsEquipped;
    CanBeEquipped = OtherEquipment->CanBeEquipped;
  }
}

UWotItem* UWotItemEquipment::Clone(UObject* Outer, const UWotItem* Item) {
  // create the new object
  UWotItemEquipment* NewEquipment = NewObject<UWotItemEquipment>(Outer, UWotItemEquipment::StaticClass());
  // Copy the properties
  NewEquipment->Copy(this);
  return NewEquipment;
}

void UWotItemEquipment::Use(ACharacter* Character)
{
  // Make sure this is still a valid character
  if (!Character) {
    UE_LOG(LogTemp, Warning, TEXT("Character is null!"));
    return;
  }
  if (UseAddedToInventory(Character)) {
    UE_LOG(LogTemp, Warning, TEXT("Added to inventory!"));
    return;
  }
  if (!CanBeEquipped) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot be used or equipped!"));
    return;
  }
  // equip it to the player
  // we don't destroy the item when we use it
  UWotEquipmentComponent* EquipmentComp = UWotEquipmentComponent::GetEquipment(Character);
  if (!EquipmentComp) {
    UE_LOG(LogTemp, Warning, TEXT("No EquipmentComponent!"));
    return;
  }
  // Start the equip/unequip process with the equipment component
  if (IsEquipped) {
    EquipmentComp->UnequipItem(this);
  } else {
    EquipmentComp->EquipItem(this);
  }
}

void UWotItemEquipment::Equip(ACharacter* Character)
{
  if (!CanBeEquipped) {
    UE_LOG(LogTemp, Warning, TEXT("Cannot be equipped, not equipping!"));
    return;
  }
  if (IsEquipped) {
    UE_LOG(LogTemp, Warning, TEXT("Already equipped, not equipping!"));
    return;
  }
  // Create the ItemActor from the ItemActorClass and attach to the character's EquipSocketName
  // Now attach it to the character's skeletal mesh
  if (!ensure(Character)) {
    UE_LOG(LogTemp, Warning, TEXT("WotItemEquipment::Equip Invalid Character!"));
    return;
  }
  USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
  if (!ensure(CharacterMesh)) {
	  UE_LOG(LogTemp, Warning, TEXT("No Mesh!"));
	  return;
  }
  if (!ensure(ItemActorClass)) {
    UE_LOG(LogTemp, Error, TEXT("WotItemEquipment::Equip Invalid ItemActorClass!"));
    return;
  }
  // Attachment rules for the meshes to the sockets
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  // we don't care about location / rotation because AttachTo will attach accordingly
  ItemActor = GetWorld()->SpawnActor<AWotItemActor>(ItemActorClass,
                                                    FVector(),
                                                    FRotator::ZeroRotator,
                                                    SpawnParams);
  ItemActor->SetItem(this);
  // Set attachment point of owner
  FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget,
                                            EAttachmentRule::SnapToTarget,
                                            EAttachmentRule::KeepWorld,
                                            true);
  ItemActor->AttachToComponent(CharacterMesh, AttachmentRules, EquipSocketName);


  // now update our properties
  UseActionText = FText::FromString("Unequip");
  IsEquipped = true;
}

void UWotItemEquipment::Unequip(ACharacter* Character)
{
  // Update our properties
  UseActionText = FText::FromString("Equip");
  IsEquipped = false;
  // Delete the ItemActor
  if (ItemActor) {
    ItemActor->Destroy();
  }
  ItemActor = nullptr;
}
