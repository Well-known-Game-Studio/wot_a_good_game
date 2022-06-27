// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/WotItemInteractibleActor.h"
#include "WotInventoryComponent.h"

// Sets default values
AWotItemInteractibleActor::AWotItemInteractibleActor() : AWotItemActor()
{
}

void AWotItemInteractibleActor::Interact_Implementation(APawn* InstigatorPawn)
{
  // get inventory component from the pawn
  UWotInventoryComponent* InventoryComp = UWotInventoryComponent::GetInventory(InstigatorPawn);
  if (InventoryComp && Item) {
    // add this object's Item to that inventory component
    InventoryComp->AddItem(Item);
  }
  // destroy this object
  Destroy();
}
