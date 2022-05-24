// Fill out your copyright notice in the Description page of Project Settings.


#include "WotAttributeComponent.h"

// Sets default values for this component's properties
UWotAttributeComponent::UWotAttributeComponent()
{
	Health = HealthMax;
	Stamina = StaminaMax;
	Strength = StrengthMax;
}

bool UWotAttributeComponent::IsAlive() const
{
	return Health > 0.0f;
}

bool UWotAttributeComponent::IsStunned() const
{
	return bIsStunned;
}

bool UWotAttributeComponent::ApplyHealthChange(float Delta)
{
	// we cannot be damaged if we are currently stunned
	if (bIsStunned && Delta < 0.0f) {
		return false;
	}
	const auto PriorHealth = Health;
	Health = std::clamp(Health+Delta, 0.0f, HealthMax);
	if (PriorHealth != Health) {
		OnHealthChanged.Broadcast(nullptr, this, Health, Delta);
		if (Health <= 0.0f) {
			// Health drops to or below 0, trigger kill event
			OnKilled.Broadcast(nullptr, this);
		}
	}
	if (Delta < 0.0f) {
		// if we are being damaged, set stunned flag so we can't get damaged
		// again too soon
		bIsStunned = true;
		// set the timer to reset stunned flag after the duration has elapsed
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Stunned,
											   this,
											   &UWotAttributeComponent::Stunned_TimeElapsed,
											   StunDuration);
	} else if (Delta > 0.0f) {
		// if we are being healed, reset the stunned flag
		bIsStunned = false;
		// and cancel the stunned timer
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Stunned);
	}
	return true;
}

void UWotAttributeComponent::Stunned_TimeElapsed()
{
	bIsStunned = false;
}
