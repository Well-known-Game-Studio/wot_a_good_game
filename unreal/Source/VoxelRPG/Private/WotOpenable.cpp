// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenable.h"
#include "Components/AudioComponent.h"

// Sets default values
AWotOpenable::AWotOpenable()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  BaseSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("BaseSceneComp"));
  RootComponent = BaseSceneComp;

  EffectAudioComp = CreateDefaultSubobject<UAudioComponent>("EffectAudioComp");
  EffectAudioComp->SetupAttachment(RootComponent);
}

void AWotOpenable::Interact_Implementation(APawn* InstigatorPawn)
{
  if (bIsOpen && bCanBeClosed) {
    bIsOpen = false;
  } else if (!bIsOpen && bCanBeOpened) {
    bIsOpen = true;
  }
  if (bIsOpen) {
    // play open sound
    EffectAudioComp->SetSound(OpenSound);
    EffectAudioComp->Play(0);
  } else {
    // play close sound
    EffectAudioComp->SetSound(CloseSound);
    EffectAudioComp->Play(0);
  }
}

// Called when the game starts or when spawned
void AWotOpenable::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotOpenable::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
