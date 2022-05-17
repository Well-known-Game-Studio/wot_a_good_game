// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemGate.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotItemGate::AWotItemGate()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  BaseSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("BaseSceneComp"));
  RootComponent = BaseSceneComp;

  LeftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftMesh"));
  LeftMesh->SetupAttachment(BaseSceneComp);

  RightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightMesh"));
  RightMesh->SetupAttachment(BaseSceneComp);
}

void AWotItemGate::Interact_Implementation(APawn* InstigatorPawn)
{
  if (bIsOpen) {
    LeftMesh->AddLocalRotation(TargetRotation);
    RightMesh->AddLocalRotation(TargetRotation.GetInverse());
  } else {
    LeftMesh->AddLocalRotation(TargetRotation.GetInverse());
    RightMesh->AddLocalRotation(TargetRotation);
  }
  bIsOpen = !bIsOpen;
}

// Called when the game starts or when spawned
void AWotItemGate::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotItemGate::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
