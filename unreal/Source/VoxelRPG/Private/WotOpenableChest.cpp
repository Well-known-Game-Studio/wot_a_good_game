// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableChest.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"

// Sets default values
AWotOpenableChest::AWotOpenableChest() : AWotOpenable()
{
  BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
  BaseMesh->SetupAttachment(RootComponent);

  LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
  LidMesh->SetupAttachment(BaseMesh);

	InventoryComp = CreateDefaultSubobject<UWotInventoryComponent>("InventoryComp");
}

void AWotOpenableChest::Interact_Implementation(APawn* InstigatorPawn)
{
  Super::Interact_Implementation(InstigatorPawn);
  LidMesh->SetRelativeRotation(FRotator(TargetPitch, 0, 0));
}

// Called when the game starts or when spawned
void AWotOpenableChest::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotOpenableChest::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
