#include "Items/WotItemWeapon.h"
#include "Items/WotEquippedWeaponActor.h"

UWotItemWeapon::UWotItemWeapon() : UWotItemEquipment()
{
  DamageAmount = 10.0f;
  // Set default weapon socket
  EquipSocketName = "Hand_R";
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
