// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableGate.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotOpenableGate::AWotOpenableGate() : AWotOpenable()
{
  LeftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftMesh"));
  LeftMesh->SetupAttachment(BaseSceneComp);

  RightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightMesh"));
  RightMesh->SetupAttachment(BaseSceneComp);
}

void AWotOpenableGate::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  LeftMesh->SetRenderCustomDepth(Enabled);
  RightMesh->SetRenderCustomDepth(Enabled);
  LeftMesh->SetCustomDepthStencilValue(HighlightValue);
  RightMesh->SetCustomDepthStencilValue(HighlightValue);
}

void AWotOpenableGate::Open_Implementation(APawn* InstigatorPawn)
{
  const bool bWasOpen = bIsOpen;
  Super::Open_Implementation(InstigatorPawn);
  // only move the meshes if the base class actually opened us
  if (!bWasOpen && bIsOpen) {
    LeftMesh->AddLocalRotation(TargetRotation);
    RightMesh->AddLocalRotation(TargetRotation.GetInverse());
  }
}

void AWotOpenableGate::Close_Implementation(APawn* InstigatorPawn)
{
  const bool bWasOpen = bIsOpen;
  Super::Close_Implementation(InstigatorPawn);
  // only move the meshes if the base class actually closed us
  if (bWasOpen && !bIsOpen) {
    LeftMesh->AddLocalRotation(TargetRotation.GetInverse());
    RightMesh->AddLocalRotation(TargetRotation);
  }
}
