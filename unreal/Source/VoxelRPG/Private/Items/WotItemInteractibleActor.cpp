// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/WotItemInteractibleActor.h"
#include "Items/WotItem.h"
#include "WotInventoryComponent.h"
#include "WotCharacter.h"

// Sets default values
AWotItemInteractibleActor::AWotItemInteractibleActor() : AWotItemActor()
{
}

void AWotItemInteractibleActor::Interact_Implementation(APawn* InstigatorPawn)
{
  // get inventory component from the pawn
  UWotInventoryComponent* InventoryComp = UWotInventoryComponent::GetInventory(InstigatorPawn);
  if (InventoryComp && Item) {
    int32 TotalCount = Item->Count;
    // add this object's Item to that inventory component
    int32 NumAdded = InventoryComp->AddItem(Item);
    if (NumAdded == 0) {
      return;
    }
    // show popup widget if it's a wotcharacter
    AWotCharacter* WotCharacter = Cast<AWotCharacter>(InstigatorPawn);
    if (WotCharacter) {
      WotCharacter->ShowPopupWidgetNumber(NumAdded, 1.0f);
    }
    if (NumAdded == TotalCount) {
      UE_LOG(LogTemp, Warning, TEXT("Nothing left to see here, folks!"));
      // destroy this object
      Destroy();
    }
  }
}
