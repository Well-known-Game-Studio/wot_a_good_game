#include "Items/WotItemWeapon.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"

UWotItemWeapon::UWotItemWeapon()
{
  UseActionText = FText::FromString("Equip");
  DamageAmount = 10.0f;
  Count = 1;
  // TODO: do we want a max count of one for weapons?
  MaxCount = 1;
	EquipSocketName = "Hand_R";
}

void UWotItemWeapon::Copy(const UWotItem* OtherItem) {
  Super::Copy(OtherItem);
  // Specialization
  const UWotItemWeapon* OtherWeapon = Cast<UWotItemWeapon>(OtherItem);
  if (OtherWeapon) {
    DamageAmount = OtherWeapon->DamageAmount;
    EquipSocketName = OtherWeapon->EquipSocketName;
  }
}

UWotItem* UWotItemWeapon::Clone(UObject* Outer, const UWotItem* Item) {
  // create the new object
  UWotItemWeapon* NewWeapon = NewObject<UWotItemWeapon>(Outer, UWotItemWeapon::StaticClass());
  // Copy the properties
  NewWeapon->Copy(this);
  return NewWeapon;
}

void UWotItemWeapon::Use(ACharacter* Character)
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
  // TODO: equip it to the player
  // we don't destroy the item when we use it
}
