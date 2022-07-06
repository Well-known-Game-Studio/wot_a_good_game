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

FText UWotGameplayFunctionLibrary::GetFloatAsTextWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero)
{
  FNumberFormattingOptions NumberFormat;
  NumberFormat.MinimumIntegralDigits = (IncludeLeadingZero) ? 1 : 0;
  NumberFormat.MaximumIntegralDigits = 10000;
  NumberFormat.MinimumFractionalDigits = Precision;
  NumberFormat.MaximumFractionalDigits = Precision;
  return FText::AsNumber(TheFloat, &NumberFormat);
}

FString UWotGameplayFunctionLibrary::GetFloatAsStringWithPrecision(float TheFloat, int32 Precision, bool IncludeLeadingZero)
{
  return GetFloatAsTextWithPrecision(TheFloat, Precision, IncludeLeadingZero).ToString();
}

FText UWotGameplayFunctionLibrary::GetIntAsText(int TheNumber)
{
  FNumberFormattingOptions NumberFormat;
  NumberFormat.MinimumIntegralDigits = 1;
  NumberFormat.MaximumIntegralDigits = 10000;
  NumberFormat.MinimumFractionalDigits = 0;
  NumberFormat.MaximumFractionalDigits = 0;
  return FText::AsNumber(TheNumber, &NumberFormat);
}

FString UWotGameplayFunctionLibrary::GetIntAsString(int TheNumber)
{
  return GetIntAsText(TheNumber).ToString();
}
