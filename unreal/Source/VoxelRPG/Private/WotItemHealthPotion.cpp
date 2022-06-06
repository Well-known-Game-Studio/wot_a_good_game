// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemHealthPotion.h"
#include "WotAttributeComponent.h"

void AWotItemHealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
  // get the attribute component of the instigating pawn
  UWotAttributeComponent* AttributeComp =
    Cast<UWotAttributeComponent>(InstigatorPawn->GetComponentByClass(UWotAttributeComponent::StaticClass()));
  // if there's no attribute component, return
  if (!AttributeComp) {
    return;
  }
  // if the pawn is at full health, do nothing
  if (AttributeComp->GetHealth() == AttributeComp->GetHealthMax()) {
    return;
  }
  // apply healing amount
  AttributeComp->ApplyHealthChange(HealingAmount);
  // Now prevent further interaction
  HideAndCooldownPowerup();
}
