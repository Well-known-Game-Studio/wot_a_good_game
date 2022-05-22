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

bool UWotAttributeComponent::ApplyHealthChange(float Delta)
{
	Health += std::clamp(Health+Delta, 0.0f, HealthMax);
	OnHealthChanged.Broadcast(nullptr, this, Health, Delta);
	return true;
}
