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
