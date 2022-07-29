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

void AWotOpenableGate::Interact_Implementation(APawn* InstigatorPawn)
{
  Super::Interact_Implementation(InstigatorPawn);
  if (bIsOpen) {
    LeftMesh->AddLocalRotation(TargetRotation);
    RightMesh->AddLocalRotation(TargetRotation.GetInverse());
  } else {
    LeftMesh->AddLocalRotation(TargetRotation.GetInverse());
    RightMesh->AddLocalRotation(TargetRotation);
  }
}
