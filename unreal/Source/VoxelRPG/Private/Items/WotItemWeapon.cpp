#include "Items/WotItemWeapon.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"

UWotItemWeapon::UWotItemWeapon()
{
}

void UWotItemWeapon::Use(ACharacter* Character)
{
  // Make sure this is still a valid character
  if (!Character) {
    UE_LOG(LogTemp, Warning, TEXT("Character is null!"));
    return;
  }
  // TODO: equip it to the player
  // we don't destroy the item when we use it
}

void UWotItemWeapon::Drop(ACharacter* Character, int DropCount)
{
  // remove it from the inventory
  if (OwningInventory) {
    OwningInventory->RemoveItem(this, DropCount);
  }
  // TODO: spawn into the world?
}
