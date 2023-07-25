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
    // get a random number
    float RandomNumber = FMath::FRandRange(0.0f, 1.0f);
    auto spawn_info = item->SpawnInfo;
    // if the random number is less than the spawn chance, spawn the item
    if (RandomNumber < spawn_info.Probability) {
      int min = spawn_info.MinCount;
      int max = spawn_info.MaxCount;
      // if the spawn info's min and max counts are 0, set them to the item's
      // Count
      if (min == 0 && max == 0) {
        min = item->Count;
        max = item->Count;
      }
      // set the count for spawning
      item->Count = FMath::RandRange(min, max);
      // add it to the inventory
      AddItem(item);
    }
  }
}

UWotItem* UWotInventoryComponent::FindItem(TSubclassOf<UWotItem> ItemClass)
{
  // find by predicate returns pointer to element found
  int32 Index = Items.IndexOfByPredicate([ItemClass](UWotItem* TestItem){
    UE_LOG(LogTemp, Log, TEXT("Testing to see if %s == %s"), *GetNameSafe(ItemClass), *GetNameSafe(TestItem->GetClass()));
    return ItemClass == TestItem->GetClass();
  });
  if (Index != INDEX_NONE) {
    return Items[Index];
  }
  // didn't find it, return nullptr
  return nullptr;
}

int32 UWotInventoryComponent::AddItem(UWotItem* Item)
{
  if (!Item) {
    return false;
  }

  int NumAdded = 0;
  // find by predicate returns pointer to element found
  int32 Index = Items.IndexOfByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  UE_LOG(LogTemp, Log, TEXT("Adding %d items"), Item->Count);
  if (Index != INDEX_NONE) {
    // We already have one in our inventory, so increment count and destroy the
    // old item
    UWotItem* OurItem = Items[Index];
    // increment the count of items we have like that, up to the max count we
    // can have
    NumAdded = OurItem->Add(Item->Count);
    // Inform the original item of how many were removed
    Item->Remove(NumAdded);
  } else {
    if (Item->OwningInventory) {
      // Remove from original inventory
      Item->OwningInventory->DeleteItem(Item);
    }
    // We don't have this item, so move (all of) it over to our inventory -
    // ensure the item knows what inventory it belongs to
    Item->OwningInventory = this;
    Item->World = GetWorld();
    // add it to the list
    Items.Add(Item);
    NumAdded = Item->Count;
  }

  UE_LOG(LogTemp, Log, TEXT("Added %d items"), NumAdded);

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  return NumAdded;
}

bool UWotInventoryComponent::RemoveItem(UWotItem* Item, int RemoveCount)
{
  if (!Item || Items.Num() == 0) {
    return false;
  }

  int NumRemoved = 0;
  // find by predicate returns pointer to element found
  int32 Index = Items.IndexOfByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  if (Index != INDEX_NONE) {
    UWotItem* OurItem = Items[Index];
    // decrement the count of items
    NumRemoved = OurItem->Remove(RemoveCount);
  } else {
    // It wasn't in the list, do nothing
  }

  UE_LOG(LogTemp, Log, TEXT("Removed %d items"), NumRemoved);

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  return NumRemoved > 0;
}

void UWotInventoryComponent::DeleteItem(UWotItem* Item) {
  if (!Item || Items.Num() == 0) {
    return;
  }

  int32 Index = Items.IndexOfByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  if (Index != INDEX_NONE) {
    UE_LOG(LogTemp, Log, TEXT("Deleted Item %s"), *Item->ItemDisplayName.ToString());
    Items.RemoveAt(Index);
    // Item->OwningInventory = nullptr;
    // Item->World = nullptr;
  }
  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();
}

void UWotInventoryComponent::DropAll() {
  FVector Location = GetOwner()->GetActorLocation();
  // TODO: cannot use range-based for loop here since Drop() will remove it from
  // our array
  while (Items.Num()) {
    UWotItem* Item = Items[0];
    Item->Drop(Location, Item->Count);
  }
}

UWotInventoryComponent* UWotInventoryComponent::GetInventory(AActor* FromActor)
{
	if (FromActor) {
		return Cast<UWotInventoryComponent>(FromActor->GetComponentByClass(UWotInventoryComponent::StaticClass()));
	}
	return nullptr;
}
