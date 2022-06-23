#include "Items/WotItemWeapon.h"

UWotItemWeapon::UWotItemWeapon() : UWotItemEquipment()
{
  DamageAmount = 10.0f;
}

void UWotItemWeapon::Copy(const UWotItem* OtherItem) {
  Super::Copy(OtherItem);
  // Specialization
  const UWotItemWeapon* OtherWeapon = Cast<UWotItemWeapon>(OtherItem);
  if (OtherWeapon) {
    DamageAmount = OtherWeapon->DamageAmount;
  }
}

UWotItem* UWotItemWeapon::Clone(UObject* Outer, const UWotItem* Item) {
  // create the new object
  UWotItemWeapon* NewWeapon = NewObject<UWotItemWeapon>(Outer, UWotItemWeapon::StaticClass());
  // Copy the properties
  NewWeapon->Copy(this);
  return NewWeapon;
}
