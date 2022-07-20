// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableDoor.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotOpenableDoor::AWotOpenableDoor() : AWotOpenable()
{
  DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
  DoorMesh->SetupAttachment(RootComponent);
}

void AWotOpenableDoor::Interact_Implementation(APawn* InstigatorPawn)
{
  Super::Interact_Implementation(InstigatorPawn);
  if (bIsOpen) {
    DoorMesh->AddLocalRotation(TargetRotation);
  } else {
    DoorMesh->AddLocalRotation(TargetRotation.GetInverse());
  }
}

// Called when the game starts or when spawned
void AWotOpenableDoor::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotOpenableDoor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
