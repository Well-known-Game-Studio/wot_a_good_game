#include "WotInventoryComponent.h"
#include "Items/WotItem.h"

UWotInventoryComponent::UWotInventoryComponent()
{
}

void UWotInventoryComponent::BeginPlay()
{
  Super::BeginPlay();
  // Start the owning actor with the default items
  for (auto& item : DefaultItems) {
    AddItem(item);
  }
}

bool UWotInventoryComponent::AddItem(UWotItem* Item)
{
  if (!Item) {
    return false;
  }

  int NumAdded = 0;
  // find by predicate returns pointer to element found
  UWotItem** OurItem = Items.FindByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  if (OurItem) {
    // increment the count of items we have like that, up to the max count we
    // can have;
    NumAdded = (*OurItem)->Add(Item->Count);
    // TODO: should we destroy the passed in item?
    Item->ConditionalBeginDestroy();
  } else {
    // Ensure the item knows what inventory it belongs to
    Item->OwningInventory = this;
    Item->World = GetWorld();
    // add it to the list
    Items.Add(Item);
    NumAdded = Item->Count;
  }

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  // TODO: what do we do if NumAdded < Item->Count?
  return NumAdded > 0;
}

bool UWotInventoryComponent::RemoveItem(UWotItem* Item, int RemoveCount)
{
  if (!Item || Items.Num() == 0) {
    return false;
  }

  // TODO: for now the item will always exist ing the inventory, registering
  // that it has been picked up, even if the count goes to 0; Should this be the
  // case? If so, do we ever actually _remove_ items from the inventory?

  // Item->OwningInventory = nullptr;
  // Item->World = nullptr;

  int NumRemoved = 0;
  // find by predicate returns pointer to element found
  UWotItem** OurItem = Items.FindByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  if (OurItem) {
    // decrement the count of items
    NumRemoved = (*OurItem)->Remove(RemoveCount);
  } else {
    // It wasn't in the list, do nothing
  }

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  // TODO: what do we do if NumRemoved < Item->Count?
  return NumRemoved > 0;
}

UWotInventoryComponent* UWotInventoryComponent::GetInventory(AActor* FromActor)
{
	if (FromActor) {
		return Cast<UWotInventoryComponent>(FromActor->GetComponentByClass(UWotInventoryComponent::StaticClass()));
	}
	return nullptr;
}
