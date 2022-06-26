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

UWotItem* UWotInventoryComponent::FindItem(TSubclassOf<UWotItem> ItemClass)
{
  // find by predicate returns pointer to element found
  int32 Index = Items.IndexOfByPredicate([ItemClass](UWotItem* TestItem){
    return ItemClass == TestItem->GetClass();
  });
  if (Index != INDEX_NONE) {
    return Items[Index];
  }
  // didn't find it, return nullptr
  return nullptr;
}

bool UWotInventoryComponent::AddItem(UWotItem* Item)
{
  if (!Item) {
    return false;
  }

  int NumAdded = 0;
  // find by predicate returns pointer to element found
  int32 Index = Items.IndexOfByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  UE_LOG(LogTemp, Warning, TEXT("Adding %d items"), Item->Count);
  if (Index != INDEX_NONE) {
    UWotItem* OurItem = Items[Index];
    // increment the count of items we have like that, up to the max count we
    // can have;
    NumAdded = OurItem->Add(Item->Count);
    // TODO: should we destroy the passed in item?
    UE_LOG(LogTemp, Warning, TEXT("Destroying Item!"));
    Item->ConditionalBeginDestroy();
  } else {
    // Ensure the item knows what inventory it belongs to
    Item->OwningInventory = this;
    Item->World = GetWorld();
    // add it to the list
    Items.Add(Item);
    NumAdded = Item->Count;
  }

  UE_LOG(LogTemp, Warning, TEXT("Added %d items"), NumAdded);

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

  UE_LOG(LogTemp, Warning, TEXT("Removed %d items"), NumRemoved);

  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();

  // TODO: what do we do if NumRemoved < Item->Count?
  return NumRemoved > 0;
}

void UWotInventoryComponent::DeleteItem(UWotItem* Item) {
  int32 Index = Items.IndexOfByPredicate([Item](UWotItem* TestItem){
    return *Item == *TestItem;
  });
  if (Index != INDEX_NONE) {
    Items.RemoveAt(Index);
    // Item->OwningInventory = nullptr;
    // Item->World = nullptr;
  }
  // Update UI and other interested parties
  OnInventoryUpdated.Broadcast();
}

void UWotInventoryComponent::DropAll() {
  for (auto Item : Items) {
    FVector Location = GetOwner()->GetActorLocation();
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
