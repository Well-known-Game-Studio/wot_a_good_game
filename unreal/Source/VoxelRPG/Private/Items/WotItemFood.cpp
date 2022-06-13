#include "Items/WotItemFood.h"
#include "GameFramework/Character.h"
#include "WotAttributeComponent.h"

UWotItemFood::UWotItemFood()
{
  HealingAmount = 10.0f;
}

void UWotItemFood::Use(ACharacter* Character)
{
  // Make sure this is still a valid character
  if (!Character) {
    return;
  }
  // Get the Attribute Component for this character
  UWotAttributeComponent* AttributeComp = Cast<UWotAttributeComponent>(Character->GetComponentByClass(UWotAttributeComponent::StaticClass()));
  if (AttributeComp) {
    return;
  }
  // And Heal Them
  AttributeComp->ApplyHealthChange(HealingAmount);
}
