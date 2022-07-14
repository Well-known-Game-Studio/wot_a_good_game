#include "WotAction.h"
#include "WotActionComponent.h"

UWotAction::UWotAction()
{

}

void UWotAction::Start_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Warning, TEXT("Running: %s"), *GetNameSafe(this));

  UWotActionComponent* Comp = GetOwningComponent();
  if (!ensure(Comp)) {
    UE_LOG(LogTemp, Error, TEXT("Owning ActionComponent is null!"));
    return;
  }
  Comp->ActiveGameplayTags.AppendTags(GrantsTags);
}

void UWotAction::Stop_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Warning, TEXT("Stopping   : %s"), *GetNameSafe(this));

  UWotActionComponent* Comp = GetOwningComponent();
  if (!ensure(Comp)) {
    UE_LOG(LogTemp, Error, TEXT("Owning ActionComponent is null!"));
    return;
  }
  Comp->ActiveGameplayTags.RemoveTags(GrantsTags);
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

UWotActionComponent* UWotAction::GetOwningComponent() const
{
  return Cast<UWotActionComponent>(GetOuter());
}
