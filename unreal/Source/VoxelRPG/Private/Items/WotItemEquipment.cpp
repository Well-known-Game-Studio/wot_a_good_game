#include "Items/WotItemEquipment.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"
#include "WotEquipmentComponent.h"

UWotItemEquipment::UWotItemEquipment()
{
  UseActionText = FText::FromString("Equip");
  Count = 1;
  // TODO: do we want a max count of one for weapons?
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
  // equip it to the player
  // we don't destroy the item when we use it
  UWotEquipmentComponent* EquipmentComp = UWotEquipmentComponent::GetEquipment(Character);
  if (!EquipmentComp) {
    UE_LOG(LogTemp, Warning, TEXT("No EquipmentComponent!"));
    return;
  }
  if (IsEquipped) {
    UseActionText = FText::FromString("Equip");
    EquipmentComp->UnequipItem(this);
  } else {
    UseActionText = FText::FromString("Unequip");
    EquipmentComp->EquipItem(this);
  }
}
