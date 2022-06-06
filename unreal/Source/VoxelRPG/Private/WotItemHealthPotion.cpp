// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemHealthPotion.h"
#include "WotAttributeComponent.h"

void AWotItemHealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
  if (!ensure(InstigatorPawn)) {
    return;
  }
  // get the attribute component of the instigating pawn
  UWotAttributeComponent* AttributeComp =
    Cast<UWotAttributeComponent>(InstigatorPawn->GetComponentByClass(UWotAttributeComponent::StaticClass()));
  // if there's no attribute component, return
  if (!AttributeComp) {
    return;
  }
  // if the pawn is at full health, do nothing
  if (AttributeComp->IsFullHealth()) {
    return;
  }
  // apply healing amount
  if (AttributeComp->ApplyHealthChange(HealingAmount)) {
    // If we successfully healed, go into cooldown
    HideAndCooldownPowerup();
  }
}
