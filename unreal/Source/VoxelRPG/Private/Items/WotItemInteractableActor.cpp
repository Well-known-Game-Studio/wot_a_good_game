// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/WotItemInteractableActor.h"
#include "Items/WotItem.h"
#include "WotInventoryComponent.h"
#include "WotCharacter.h"

// Sets default values
AWotItemInteractableActor::AWotItemInteractableActor() : AWotItemActor()
{
}

void AWotItemInteractableActor::Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit)
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
      WotCharacter->PlaySoundGet();
    }
    if (NumAdded == TotalCount) {
      UE_LOG(LogTemp, Log, TEXT("InteractableActor: We've added all our items, destroying!"));
      // destroy this object
      Destroy();
    }
  }
}

void AWotItemInteractableActor::GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText)
{
  if (Item) {
    OutText = FText::Format(NSLOCTEXT("WotItemInteractableActor", "PickupFormat", "Pick up {0}"), Item->ItemDisplayName);
  }
  else {
    OutText = NSLOCTEXT("WotItemInteractableActor", "Pickup", "Pick up");
  }
}

void AWotItemInteractableActor::Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration=0)
{
  SetHighlightEnabled(HighlightValue, true);
  // if duration is > 0, start a timer to unhighlight the object
  if (Duration > 0) {
    GetWorldTimerManager().SetTimer(HighlightTimerHandle, this, &AWotItemInteractableActor::OnHighlightTimerExpired, Duration, false);
  }
}

void AWotItemInteractableActor::Unhighlight_Implementation(FHitResult Hit)
{
  SetHighlightEnabled(0, false);
}

void AWotItemInteractableActor::OnHighlightTimerExpired()
{
  // dummy hit
  FHitResult Hit;
  IWotGameplayInterface::Execute_Unhighlight(this, Hit);
}

void AWotItemInteractableActor::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  Mesh->SetRenderCustomDepth(Enabled);
  Mesh->SetCustomDepthStencilValue(HighlightValue);
}
