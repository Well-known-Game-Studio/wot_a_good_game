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

void AWotOpenableGate::Open_Implementation(APawn* InstigatorPawn)
{
  Super::Open_Implementation(InstigatorPawn);
  LeftMesh->AddLocalRotation(TargetRotation);
  RightMesh->AddLocalRotation(TargetRotation.GetInverse());
}

void AWotOpenableGate::Close_Implementation(APawn* InstigatorPawn)
{
  Super::Close_Implementation(InstigatorPawn);
  LeftMesh->AddLocalRotation(TargetRotation.GetInverse());
  RightMesh->AddLocalRotation(TargetRotation);
}
