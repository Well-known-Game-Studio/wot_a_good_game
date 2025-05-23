// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenable.h"
#include "Components/AudioComponent.h"

// Sets default values
AWotOpenable::AWotOpenable()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = false;

  BaseSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("BaseSceneComp"));
  RootComponent = BaseSceneComp;

  EffectAudioComp = CreateDefaultSubobject<UAudioComponent>("EffectAudioComp");
  EffectAudioComp->SetupAttachment(RootComponent);
}

void AWotOpenable::BeginPlay()
{
  Super::BeginPlay();
  // Assume that the door is closed at the beginning, so we need to open it if
  // it is open
  if (bIsOpen) {
    Open(nullptr);
  }
}

void AWotOpenable::Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit)
{
  if (bIsOpen) {
    Close(InstigatorPawn);
  } else {
    Open(InstigatorPawn);
  }
}

void AWotOpenable::GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText)
{
  if (bIsOpen) {
    OutText = CloseText;
  } else {
    OutText = OpenText;
  }
}

void AWotOpenable::Highlight_Implementation(FHitResult Hit, int HighlightValue, float Duration=0)
{
  SetHighlightEnabled(HighlightValue, true);
  // if duration is > 0, start a timer to unhighlight the object
  if (Duration > 0) {
    GetWorldTimerManager().SetTimer(HighlightTimerHandle, this, &AWotOpenable::OnHighlightTimerExpired, Duration, false);
  }
}

void AWotOpenable::Unhighlight_Implementation(FHitResult Hit)
{
  SetHighlightEnabled(0, false);
}

void AWotOpenable::OnHighlightTimerExpired()
{
  // dummy hit
  FHitResult Hit;
  IWotGameplayInterface::Execute_Unhighlight(this, Hit);
}

void AWotOpenable::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  // Let the subclasses handle the highlighting
}

void AWotOpenable::Open_Implementation(APawn* InstigatorPawn)
{
  if (bCanBeOpened && !bIsOpen) {
    // only update the state if it was closed
    bIsOpen = true;
    OnOpened.Broadcast(InstigatorPawn, this);
    OnStateChanged.Broadcast(InstigatorPawn, this, bIsOpen);
    // play open sound
    EffectAudioComp->SetSound(OpenSound);
    EffectAudioComp->Play(0);
  }
}

void AWotOpenable::Close_Implementation(APawn* InstigatorPawn)
{
  if (bCanBeClosed && bIsOpen) {
    // only update the state if it was open
    bIsOpen = false;
    OnClosed.Broadcast(InstigatorPawn, this);
    OnStateChanged.Broadcast(InstigatorPawn, this, bIsOpen);
    // play close sound
    EffectAudioComp->SetSound(CloseSound);
    EffectAudioComp->Play(0);
  }
}
