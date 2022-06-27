#include "Items/WotItemWeapon.h"
#include "Items/WotEquippedWeaponActor.h"

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

bool UWotItemWeapon::PrimaryAttackStart_Implementation()
{
  if (ItemActor) {
    AWotEquippedWeaponActor* WeaponActor = Cast<AWotEquippedWeaponActor>(ItemActor);
    if (WeaponActor) {
      WeaponActor->PrimaryAttackStart();
    }
  }
  return true;
}

bool UWotItemWeapon::PrimaryAttackStop_Implementation()
{
  if (ItemActor) {
    AWotEquippedWeaponActor* WeaponActor = Cast<AWotEquippedWeaponActor>(ItemActor);
    if (WeaponActor) {
      WeaponActor->PrimaryAttackStop();
    }
  }
  return true;
}

bool UWotItemWeapon::SecondaryAttackStart_Implementation()
{
  if (ItemActor) {
    AWotEquippedWeaponActor* WeaponActor = Cast<AWotEquippedWeaponActor>(ItemActor);
    if (WeaponActor) {
      WeaponActor->SecondaryAttackStart();
    }
  }
  return true;
}

bool UWotItemWeapon::SecondaryAttackStop_Implementation()
{
  if (ItemActor) {
    AWotEquippedWeaponActor* WeaponActor = Cast<AWotEquippedWeaponActor>(ItemActor);
    if (WeaponActor) {
      WeaponActor->SecondaryAttackStop();
    }
  }
  return true;
}
