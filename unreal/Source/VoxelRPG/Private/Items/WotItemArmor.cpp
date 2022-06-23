#include "Items/WotItemArmor.h"

UWotItemArmor::UWotItemArmor() : UWotItemEquipment()
{
  ArmorAmount = 10.0f;
}

void UWotItemArmor::Copy(const UWotItem* OtherItem) {
  Super::Copy(OtherItem);
  // Specialization
  const UWotItemArmor* OtherArmor = Cast<UWotItemArmor>(OtherItem);
  if (OtherArmor) {
    ArmorAmount = OtherArmor->ArmorAmount;
  }
}

UWotItem* UWotItemArmor::Clone(UObject* Outer, const UWotItem* Item) {
  // create the new object
  UWotItemArmor* NewArmor = NewObject<UWotItemArmor>(Outer, UWotItemArmor::StaticClass());
  // Copy the properties
  NewArmor->Copy(this);
  return NewArmor;
}
