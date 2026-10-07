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

void AWotOpenableGate::SetOpenVisuals(bool bOpen)
{
  // Rotation is applied incrementally, so this must only be called on a real
  // state transition (which the base class guarantees).
  const FRotator Rotation = bOpen ? TargetRotation : TargetRotation.GetInverse();
  LeftMesh->AddLocalRotation(Rotation);
  RightMesh->AddLocalRotation(Rotation.GetInverse());
}
