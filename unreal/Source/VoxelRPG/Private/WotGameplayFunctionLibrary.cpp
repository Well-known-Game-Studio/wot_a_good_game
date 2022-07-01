#include "WotGameplayFunctionLibrary.h"
#include "WotAttributeComponent.h"

bool UWotGameplayFunctionLibrary::ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount)
{
  UWotAttributeComponent* AttributeComp = UWotAttributeComponent::GetAttributes(TargetActor);
  if (AttributeComp) {
    return AttributeComp->ApplyHealthChangeInstigator(DamageCauser, DamageAmount);
  }
  return false;
}

bool UWotGameplayFunctionLibrary::ApplyDirectionalDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount, const FHitResult& HitResult)
{
  if (ApplyDamage(DamageCauser, TargetActor, DamageAmount)) {
    UPrimitiveComponent* HitComp = HitResult.GetComponent();
    if (HitComp && HitComp->IsSimulatingPhysics(HitResult.BoneName)) {
      // TODO: right now our NPC meshes don't support physics collision at all,
      // so this doesn't work
      HitComp->AddImpulseAtLocation(-HitResult.Normal * 3000.0f, HitResult.ImpactPoint, HitResult.BoneName);
    }
    return true;
  }
  return false;
}
