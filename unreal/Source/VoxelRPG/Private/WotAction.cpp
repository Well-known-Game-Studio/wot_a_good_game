#include "WotAction.h"
#include "WotActionComponent.h"

UWotAction::UWotAction()
{

}

bool UWotAction::CanStart(AActor* Instigator)
{
  if (IsRunning()) {
    return false;
  }

  UWotActionComponent* Comp = GetOwningComponent();

  if (Comp->ActiveGameplayTags.HasAny(BlockedTags)) {
    return false;
  }
  if (!Comp->ActiveGameplayTags.HasAll(RequiredTags)) {
    return false;
  }
  return true;
}

void UWotAction::Start_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Log, TEXT("Running: %s"), *GetNameSafe(this));

  UWotActionComponent* Comp = GetOwningComponent();
  if (!ensure(Comp)) {
    UE_LOG(LogTemp, Error, TEXT("Owning ActionComponent is null!"));
    return;
  }
  // add the tags we grant
  Comp->ActiveGameplayTags.AppendTags(GrantsTags);
  // remove the tags we remove
  Comp->ActiveGameplayTags.RemoveTags(RemovesTags);
  // make sure to update the running flag
  bIsRunning = true;
}

void UWotAction::Stop_Implementation(AActor* Instigator)
{
  UE_LOG(LogTemp, Log, TEXT("Stopping: %s"), *GetNameSafe(this));

  ensureAlways(bIsRunning);

  UWotActionComponent* Comp = GetOwningComponent();
  if (!ensure(Comp)) {
    UE_LOG(LogTemp, Error, TEXT("Owning ActionComponent is null!"));
    return;
  }
  // remove the tags we grant
  Comp->ActiveGameplayTags.RemoveTags(GrantsTags);
  // make sure to update the running flag
  bIsRunning = false;
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

bool UWotAction::IsRunning() const
{
  return bIsRunning;
}
