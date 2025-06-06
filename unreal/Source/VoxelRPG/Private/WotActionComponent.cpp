#include "WotActionComponent.h"
#include "WotAction.h"

UWotActionComponent::UWotActionComponent()
{

}

void UWotActionComponent::BeginPlay()
{
  Super::BeginPlay();
  // Start the owning actor with the default actions
  for (auto& ActionClass : DefaultActions) {
    AddAction(ActionClass);
  }
}

void UWotActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  FString DebugMsg = GetNameSafe(GetOwner()) + " : " + ActiveGameplayTags.ToStringSimple();
  GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugMsg);
}

void UWotActionComponent::AddAction(TSubclassOf<UWotAction> ActionClass)
{
  if (!ensure(ActionClass)) {
    return;
  }
  UWotAction* NewAction = NewObject<UWotAction>(this, ActionClass);
  if (ensure(NewAction)) {
    Actions.Add(NewAction);
  }
}

bool UWotActionComponent::StartActionByName(FName ActionName, AActor* Instigator)
{
  if (!bEnableActions) {
    return false;
  }

  for (UWotAction* Action : Actions) {
    if (Action && Action->ActionName == ActionName) {
      if (!Action->CanStart(Instigator)) {
        FString FailedMsg = FString::Printf(TEXT("Failed to run: %s"), *ActionName.ToString());
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FailedMsg);
        // can't start this action, so continue on to the next action in the array
        continue;
      }
      Action->Start(Instigator);
      return true;
    }
  }
  return false;
}

bool UWotActionComponent::StopActionByName(FName ActionName, AActor* Instigator)
{
  for (UWotAction* Action : Actions) {
    if (Action && Action->ActionName == ActionName) {
      if (Action->IsRunning()) {
        Action->Stop(Instigator);
        return true;
      }
    }
  }
  return false;
}

bool UWotActionComponent::StopAllActions(AActor* Instigator)
{
  bool bStoppedAnyAction = false;
  for (UWotAction* Action : Actions) {
    if (Action && Action->IsRunning()) {
      Action->Stop(Instigator);
      bStoppedAnyAction = true;
    }
  }
  return bStoppedAnyAction;
}

UWotActionComponent* UWotActionComponent::GetActions(AActor* FromActor)
{
	if (FromActor) {
		return Cast<UWotActionComponent>(FromActor->GetComponentByClass(UWotActionComponent::StaticClass()));
	}
	return nullptr;
}

bool UWotActionComponent::IsActionRunning(FName ActionName) const
{
  for (const UWotAction* Action : Actions) {
    if (Action && Action->ActionName == ActionName && Action->IsRunning()) {
      return true;
    }
  }
  return false;
}

bool UWotActionComponent::IsAnyActionRunning() const
{
  for (const UWotAction* Action : Actions) {
    if (Action && Action->IsRunning()) {
      return true;
    }
  }
  return false;
}
