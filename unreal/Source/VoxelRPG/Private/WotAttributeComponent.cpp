// Fill out your copyright notice in the Description page of Project Settings.


#include "WotAttributeComponent.h"

// Sets default values for this component's properties
UWotAttributeComponent::UWotAttributeComponent()
{
	Health = 100;
}


bool UWotAttributeComponent::ApplyHealthChange(float Delta)
{
	Health += Delta;
	OnHealthChanged.Broadcast(nullptr, this, Health, Delta);
	return true;
}
