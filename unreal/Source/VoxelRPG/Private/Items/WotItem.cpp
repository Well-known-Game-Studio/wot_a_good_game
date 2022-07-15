#include "Items/WotItem.h"
#include "WotInventoryComponent.h"
#include "WotEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "Items/WotItemInteractibleActor.h"

UWotItem::UWotItem()
{
  UseActionText = FText::FromString("Use");
  ItemDisplayName = FText::FromString("Item");
  ItemDescription = FText::FromString("Description");
  Weight = 1.0f;
  Count = 1;
  MaxCount = 0;
}

bool UWotItem::CanBeUsedBy(ACharacter* Character)
{
  if (!Character) {
    UE_LOG(LogTemp, Log, TEXT("Cannot be used, not a valid character!"));
    return false;
  }
  UWotInventoryComponent* InventoryComp = UWotInventoryComponent::GetInventory(Character);
  if (!InventoryComp) {
    UE_LOG(LogTemp, Log, TEXT("Cannot be used, not a valid inventory!"));
    return false;
  }
  if (InventoryComp != OwningInventory) {
    UE_LOG(LogTemp, Log, TEXT("Cannot be used, not owner!"));
    return false;
  }
  return true;
}

bool UWotItem::UseAddedToInventory(ACharacter* Character)
{
  if (!Character) {
    UE_LOG(LogTemp, Log, TEXT("Cannot add to inventory, invalid character!"));
    return false;
  }
  UWotInventoryComponent* NewInventory = UWotInventoryComponent::GetInventory(Cast<AActor>(Character));
  if (!NewInventory) {
    UE_LOG(LogTemp, Log, TEXT("Cannot add to inventory, invalid NewInventory!"));
    return false;
  }
  if (NewInventory == OwningInventory) {
    UE_LOG(LogTemp, Log, TEXT("Cannot add to inventory, NewInventory == OwningInventory!"));
    return false;
  }
  // Add to new character inventory
  int32 NumAdded = NewInventory->AddItem(this);
  return NumAdded > 0;
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
  UE_LOG(LogTemp, Log, TEXT("Trying to remove %d"), RemovedCount);
  if (Count <= 0) {
    return 0;
  }
  int ActualRemoved = std::min(Count, RemovedCount);
  Count -= ActualRemoved;
  if (Count == 0) {
    UE_LOG(LogTemp, Log, TEXT("Count reached 0, removing and destroying Item!"));
    if (OwningInventory) {
      OwningInventory->DeleteItem(this);
    }
    ConditionalBeginDestroy();
  }
  UE_LOG(LogTemp, Log, TEXT("Removed %d"), ActualRemoved);

  return ActualRemoved;
}

void UWotItem::Drop(FVector Location, int DropCount)
{
  if (Count <= 0) {
    UE_LOG(LogTemp, Log, TEXT("Cannot drop any more, Count <= 0"));
    return;
  }
  if (OwningInventory) {
    // Get the owning inventory's character
    AActor* Owner = OwningInventory->GetOwner();
    if (Owner) {
      // unequip the item (weapon / equipment / etc.)
      UWotEquipmentComponent* EquipmentComp = UWotEquipmentComponent::GetEquipment(Owner);
      if (EquipmentComp) {
        EquipmentComp->UnequipItem(this);
      }
    }
  }

  // spawn it into the world as a WotItemInteractibleActor
  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  // Spawn one actor for each item dropped
  for (int i=0; i < DropCount; ++i) {
    AWotItemInteractibleActor* InteractibleItem =
      GetWorld()->SpawnActor<AWotItemInteractibleActor>(AWotItemInteractibleActor::StaticClass(),
                                                        Location,
                                                        FRotator::ZeroRotator,
                                                        SpawnParams);
    // create an Item for this
    UWotItem* DroppedItem = DuplicateObject(this, InteractibleItem);
    // Set the properties of the dropped item accordingly
    DroppedItem->OwningInventory = nullptr;
    DroppedItem->Count = 1;
    InteractibleItem->SetItem(DroppedItem);
  }
  // remove it from the inventory
  if (OwningInventory) {
    OwningInventory->RemoveItem(this, DropCount);
  }
}

UWorld* UWotItem::GetWorld() const
{
  // if our World reference is valid, return that
  if (World) {
    return World;
  }
  // Outer is set when creating action via NewObject<T>
  // try casting to actor component
  UActorComponent* Comp = Cast<UActorComponent>(GetOuter());
  if (Comp) {
    return Comp->GetWorld();
  }
  // if that fails, try casting to actor
  AActor* Actor = Cast<AActor>(GetOuter());
  if (Actor) {
    return Actor->GetWorld();
  }
  // if those fail, return nullptr :/
  return nullptr;
}

bool UWotItem::operator==(const UWotItem& rhs)
{
  // only check things that identify this object, not anything else; this is
  // used when comparing for inventory insertion / removal, so we don't care
  // about owning inventory, world, or couunt
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
