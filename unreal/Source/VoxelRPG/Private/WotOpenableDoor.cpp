// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableDoor.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotOpenableDoor::AWotOpenableDoor() : AWotOpenable()
{
  DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
  DoorMesh->SetupAttachment(RootComponent);
}

void AWotOpenableDoor::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  DoorMesh->SetRenderCustomDepth(Enabled);
  DoorMesh->SetCustomDepthStencilValue(HighlightValue);
}

void AWotOpenableDoor::Open_Implementation(APawn* InstigatorPawn)
{
  Super::Open_Implementation(InstigatorPawn);
  DoorMesh->AddLocalRotation(TargetRotation);
}

void AWotOpenableDoor::Close_Implementation(APawn* InstigatorPawn)
{
  Super::Close_Implementation(InstigatorPawn);
  DoorMesh->AddLocalRotation(TargetRotation.GetInverse());
}
