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

void AWotOpenableDoor::SetOpenVisuals(bool bOpen)
{
  // Rotation is applied incrementally, so this must only be called on a real
  // state transition (which the base class guarantees).
  DoorMesh->AddLocalRotation(bOpen ? TargetRotation : TargetRotation.GetInverse());
}
