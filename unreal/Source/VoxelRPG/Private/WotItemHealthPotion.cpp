// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemHealthPotion.h"
#include "WotAttributeComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotItemHealthPotion::AWotItemHealthPotion()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
  RootComponent = BaseMesh;
}

void AWotItemHealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
  // do nothing if we're still cooling down
  if (!bIsInteractible) {
    return;
  }
  // get the attribute component of the instigating pawn
  UWotAttributeComponent* AttributeComp =
    Cast<UWotAttributeComponent>(InstigatorPawn->GetComponentByClass(UWotAttributeComponent::StaticClass()));
  // if there's no attribute component, return
  if (!AttributeComp) {
    return;
  }
  // if the pawn is at full health, do nothing
  if (AttributeComp->GetHealth() == AttributeComp->GetHealthMax()) {
    return;
  }
  // apply healing amount
  AttributeComp->ApplyHealthChange(HealingAmount);
  // set IsInteractible to false
  bIsInteractible = false;
  // start cooldown timer
	GetWorldTimerManager().SetTimer(TimerHandle_Cooldown, this, &AWotItemHealthPotion::Cooldown_TimeElapsed, CooldownTime);
}

void AWotItemHealthPotion::Cooldown_TimeElapsed()
{
  bIsInteractible = true;
}

// Called when the game starts or when spawned
void AWotItemHealthPotion::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotItemHealthPotion::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
