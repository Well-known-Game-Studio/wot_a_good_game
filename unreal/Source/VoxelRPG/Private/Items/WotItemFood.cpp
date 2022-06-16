#include "Items/WotItemFood.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"

UWotItemFood::UWotItemFood()
{
  HealingAmount = 10.0f;
  Count = 1;
}

void UWotItemFood::Use(ACharacter* Character)
{
  // Make sure there are items to use
  if (Count <= 0) {
    UE_LOG(LogTemp, Warning, TEXT("No more food!"));
    return;
  }
  // Make sure this is still a valid character
  if (!Character) {
    UE_LOG(LogTemp, Warning, TEXT("Character is null!"));
    return;
  }
  // Get the Attribute Component for this character
  UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(Character);
  if (!AttributeComp) {
    UE_LOG(LogTemp, Warning, TEXT("Attribute Component is null!"));
    return;
  }
  // And Heal Them
  bool WasUsed = AttributeComp->ApplyHealthChange(HealingAmount);
  if (!WasUsed) {
    UE_LOG(LogTemp, Warning, TEXT("Full health!"));
    return;
  }
  // remove it from the inventory
  if (OwningInventory) {
    OwningInventory->RemoveItem(this, 1);
  }
  // TODO: then destroy?
  // ConditionalBeginDestroy();
  // we consume the food on use, so destroy it
}

void UWotItemFood::Drop(ACharacter* Character, int DropCount)
{
  // remove it from the inventory
  if (OwningInventory) {
    OwningInventory->RemoveItem(this, DropCount);
  }
  // TODO: spawn it into the world?
}
