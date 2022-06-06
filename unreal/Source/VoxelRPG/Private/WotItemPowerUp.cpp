// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemPowerUp.h"
#include "WotAttributeComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotItemPowerUp::AWotItemPowerUp()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = false;

  BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
  RootComponent = BaseMesh;
}

void AWotItemPowerUp::Interact_Implementation(APawn* InstigatorPawn)
{
  // logic in derived classes...
}

void AWotItemPowerUp::ShowPowerup()
{
  SetPowerupState(true);
}

void AWotItemPowerUp::HideAndCooldownPowerup()
{
  SetPowerupState(false);

  GetWorldTimerManager().SetTimer(TimerHandle_Cooldown, this, &AWotItemPowerUp::ShowPowerup, CooldownTime);
}

void AWotItemPowerUp::SetPowerupState(bool bNewIsInteractible)
{
  SetActorEnableCollision(bNewIsInteractible);
  // set visibility of base mesh and all children
  BaseMesh->SetVisibility(bNewIsInteractible, true);
}
