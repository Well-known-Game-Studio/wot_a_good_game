// Fill out your copyright notice in the Description page of Project Settings.
#include "Items/WotItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Items/WotItem.h"

// Sets default values
AWotItemActor::AWotItemActor()
{
  Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
  RootComponent = Mesh;
}

void AWotItemActor::SetItem(UWotItem* NewItem) {
  Item = NewItem;
  if (!Item) {
    UE_LOG(LogTemp, Warning, TEXT("Cleared ItemActor's Item"));
    return;
  }
  // Set the item mesh
  if (Item->PickupMesh) {
    Mesh->SetStaticMesh(Item->PickupMesh);
  }
}

void AWotItemActor::SetPhysicsAndCollision(FName CollisionProfileName, bool EnablePhysics, bool EnableCollision)
{
  Mesh->SetCollisionProfileName(CollisionProfileName);
  SetActorEnableCollision(EnableCollision);
  Mesh->SetSimulatePhysics(EnablePhysics);
}
