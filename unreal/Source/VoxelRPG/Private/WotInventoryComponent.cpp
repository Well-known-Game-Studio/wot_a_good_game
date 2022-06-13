#include "WotInventoryComponent.h"
#include "Items/WotItem.h"

UWotInventoryComponent::UWotInventoryComponent()
{
  Capacity = 20;
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
  if (Items.Num() >= Capacity || !Item) {
    return false;
  }

  Item->OwningInventory = this;
  Item->World = GetWorld();
  Items.Add(Item);

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  return true;
}

bool UWotInventoryComponent::RemoveItem(UWotItem* Item)
{
  if (!Item || Items.Num() == 0) {
    return false;
  }

  Item->OwningInventory = nullptr;
  Item->World = nullptr;
  Items.RemoveSingle(Item);

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  return true;
}
