#include "WotAction.h"

UWotAction::UWotAction()
{

}

void UWotAction::Start_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Warning, TEXT("Running: %s"), *GetNameSafe(this));
}

void UWotAction::Stop_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Warning, TEXT("Stopping   : %s"), *GetNameSafe(this));
}

UWorld* UWotAction::GetWorld() const
{
  // Outer is set when creating action via NewObject<T>
  UActorComponent* Comp = Cast<UActorComponent>(GetOuter());
  if (Comp) {
    return Comp->GetWorld();
  }
  return nullptr;
}
