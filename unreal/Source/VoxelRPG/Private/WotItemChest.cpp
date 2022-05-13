// Fill out your copyright notice in the Description page of Project Settings.

#include "WotItemChest.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AWotItemChest::AWotItemChest()
{
  // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
  RootComponent = BaseMesh;

  LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
  LidMesh->SetupAttachment(BaseMesh);
}

void AWotItemChest::Interact_Implementation(APawn* InstigatorPawn)
{

}

// Called when the game starts or when spawned
void AWotItemChest::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void AWotItemChest::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
