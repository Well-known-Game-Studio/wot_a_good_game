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

void AWotItemPowerUp::Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit)
{
  // logic in derived classes...
}

void AWotItemPowerUp::GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText)
{
  // logic in derived classes...
}

void AWotItemPowerUp::Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration=0)
{
  SetHighlightEnabled(HighlightValue, true);
  // if duration is > 0, start a timer to unhighlight the object
  if (Duration > 0) {
    GetWorldTimerManager().SetTimer(HighlightTimerHandle, this, &AWotItemPowerUp::OnHighlightTimerExpired, Duration, false);
  }
}

void AWotItemPowerUp::Unhighlight_Implementation(FHitResult Hit)
{
  SetHighlightEnabled(0, false);
}

void AWotItemPowerUp::OnHighlightTimerExpired()
{
  // dummy hit
  FHitResult Hit;
  IWotGameplayInterface::Execute_Unhighlight(this, Hit);
}

void AWotItemPowerUp::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  BaseMesh->SetRenderCustomDepth(Enabled);
  BaseMesh->SetCustomDepthStencilValue(HighlightValue);
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

void AWotItemPowerUp::SetPowerupState(bool bNewIsInteractable)
{
  SetActorEnableCollision(bNewIsInteractable);
  // set visibility of base mesh and all children
  BaseMesh->SetVisibility(bNewIsInteractable, true);
}
