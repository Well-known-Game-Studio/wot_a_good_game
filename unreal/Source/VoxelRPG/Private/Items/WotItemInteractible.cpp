// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/WotItemInteractible.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"
#include "Items/WotItem.h"

// Sets default values
AWotItemInteractible::AWotItemInteractible()
{
  Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
  RootComponent = Mesh;
  // Set physics enabled on this actor
  SetPhysicsAndCollision("Item", true, true);
}

void AWotItemInteractible::SetItem(UWotItem* NewItem) {
  Item = NewItem;
  // Set the item mesh
  if (Item->PickupMesh) {
    Mesh->SetStaticMesh(Item->PickupMesh);
  }
}

void AWotItemInteractible::SetPhysicsAndCollision(FName CollisionProfileName, bool EnablePhysics, bool EnableCollision)
{
  Mesh->SetCollisionProfileName(CollisionProfileName);
  SetActorEnableCollision(EnableCollision);
  Mesh->SetSimulatePhysics(EnablePhysics);
}

void AWotItemInteractible::Interact_Implementation(APawn* InstigatorPawn)
{
  // get inventory component from the pawn
  UWotInventoryComponent* InventoryComp = UWotInventoryComponent::GetInventory(InstigatorPawn);
  if (InventoryComp && Item) {
    // add this object's Item to that inventory component
    InventoryComp->AddItem(Item);
  }
  // destroy this object
  Destroy();
}
