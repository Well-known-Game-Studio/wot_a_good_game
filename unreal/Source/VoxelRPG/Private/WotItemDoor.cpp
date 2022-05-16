// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemDoor.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotItemDoor::AWotItemDoor()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
  RootComponent = DoorMesh;
}

void AWotItemDoor::Interact_Implementation(APawn* InstigatorPawn)
{
  if (bIsOpen) {
    DoorMesh->AddLocalRotation(TargetRotation);
  } else {
    DoorMesh->AddLocalRotation(TargetRotation.GetInverse());
  }
  bIsOpen = !bIsOpen;
}

// Called when the game starts or when spawned
void AWotItemDoor::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotItemDoor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
