#include "Items/WotItem.h"
#include "WotInventoryComponent.h"
#include "GameFramework/Character.h"

UWotItem::UWotItem()
{
  UseActionText = FText::FromString("Use");
  ItemDisplayName = FText::FromString("Item");
  ItemDescription = FText::FromString("Description");
  Weight = 1.0f;
  Count = 1;
  MaxCount = 0;
}

bool UWotItem::UseAddedToInventory(ACharacter* Character)
{
  if (!Character) {
    return false;
  }
  UWotInventoryComponent* NewInventory = UWotInventoryComponent::GetInventory(Cast<AActor>(Character));
  if (!NewInventory) {
    return false;
  }
  if (NewInventory == OwningInventory) {
    return false;
  }
  // Remove from current inventory
  if (OwningInventory) {
    OwningInventory->RemoveItem(this, this->Count);
  }
  // And add to new character inventory
  NewInventory->AddItem(this);
  return true;
}

int UWotItem::Add(int AddedCount)
{
  if (MaxCount > 0) {
    // We have a maximum specified, enforce it
    if (Count >= MaxCount) {
      return 0;
    }
    int SpaceAvailable = MaxCount - Count;
    int ActualAdded = std::min(AddedCount, SpaceAvailable);
    Count += ActualAdded;
    return ActualAdded;
  } else {
    // MaxCount <= 0 implies no limit on storage
    Count += AddedCount;
    return AddedCount;
  }
}

int UWotItem::Remove(int RemovedCount)
{
  if (Count <= 0) {
    return 0;
  }
  int ActualRemoved = std::min(Count, RemovedCount);
  Count -= ActualRemoved;
  return ActualRemoved;
}

bool UWotItem::operator==(const UWotItem& rhs)
{
  // only check things that identify this object, not anything else; this is
  // used when comparing for inventory insertion / removal, so we don't care
  // about owning invetory, world, or couunt
  return
    PickupMesh == rhs.PickupMesh &&
    Thumbnail == rhs.Thumbnail &&
    ItemDisplayName.EqualTo(rhs.ItemDisplayName) &&
    ItemDescription.EqualTo(rhs.ItemDescription) &&
    Weight == rhs.Weight;
}

bool UWotItem::operator!=(const UWotItem& rhs)
{
  return !(*this == rhs);
}
