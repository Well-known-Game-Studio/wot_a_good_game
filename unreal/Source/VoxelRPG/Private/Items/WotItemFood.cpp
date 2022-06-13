#include "Items/WotItemFood.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"
#include "WotInventoryComponent.h"

UWotItemFood::UWotItemFood()
{
  HealingAmount = 10.0f;
}

void UWotItemFood::Use(ACharacter* Character)
{
  // Make sure this is still a valid character
  if (!Character) {
    UE_LOG(LogTemp, Warning, TEXT("Character is null!"));
    return;
  }
  // Get the Attribute Component for this character
  UWotAttributeComponent* AttributeComp = Cast<UWotAttributeComponent>(Character->GetComponentByClass(UWotAttributeComponent::StaticClass()));
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
    OwningInventory->RemoveItem(this);
  }
  // then destroy?
  ConditionalBeginDestroy();
}
