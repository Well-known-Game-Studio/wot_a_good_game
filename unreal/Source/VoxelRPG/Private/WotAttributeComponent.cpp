// Fill out your copyright notice in the Description page of Project Settings.


#include "WotAttributeComponent.h"

// Sets default values for this component's properties
UWotAttributeComponent::UWotAttributeComponent()
{
	Health = HealthMax;
	Stamina = StaminaMax;
	Strength = StrengthMax;
}

float UWotAttributeComponent::GetHealth() const
{
	return Health;
}

float UWotAttributeComponent::GetHealthMax() const
{
	return HealthMax;
}

bool UWotAttributeComponent::IsAlive() const
{
	return Health > 0.0f;
}

bool UWotAttributeComponent::IsStunned() const
{
	return bIsStunned;
}

bool UWotAttributeComponent::IsFullHealth() const
{
	return Health == HealthMax;
}

bool UWotAttributeComponent::ApplyHealthChange(float Delta)
{
	return ApplyHealthChangeInstigator(nullptr, Delta);
}

bool UWotAttributeComponent::ApplyHealthChangeInstigator(AActor* InstigatorActor, float Delta)
{
	// we cannot be damaged if we are currently stunned
	if (bIsStunned && Delta < 0.0f) {
		return false;
	}
	const auto PriorHealth = Health;
	Health = std::clamp(Health+Delta, 0.0f, HealthMax);
	const auto ActualDelta = Health - PriorHealth;
	if (ActualDelta != 0) {
		OnHealthChanged.Broadcast(InstigatorActor, this, Health, ActualDelta);
		if (Health <= 0.0f) {
			// Health drops to or below 0, trigger kill event
			OnKilled.Broadcast(InstigatorActor, this);
		}
	}
	if (ActualDelta < 0.0f) {
		// if we are being damaged, set stunned flag so we can't get damaged
		// again too soon
		bIsStunned = true;
		// set the timer to reset stunned flag after the duration has elapsed
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Stunned,
											   this,
											   &UWotAttributeComponent::Stunned_TimeElapsed,
											   StunDuration);
	} else if (ActualDelta > 0.0f) {
		// if we are being healed, reset the stunned flag
		bIsStunned = false;
		// and cancel the stunned timer
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Stunned);
	}
	return ActualDelta != 0;
}

void UWotAttributeComponent::Stunned_TimeElapsed()
{
	bIsStunned = false;
}

UWotAttributeComponent* UWotAttributeComponent::GetAttributes(AActor* FromActor)
{
	if (FromActor) {
		return Cast<UWotAttributeComponent>(FromActor->GetComponentByClass(UWotAttributeComponent::StaticClass()));
	}
	return nullptr;
}

bool UWotAttributeComponent::IsActorAlive(AActor* Actor)
{
	UWotAttributeComponent* AttributeComp = GetAttributes(Actor);
	if (AttributeComp) {
		return AttributeComp->IsAlive();
	}
	return false;
}
