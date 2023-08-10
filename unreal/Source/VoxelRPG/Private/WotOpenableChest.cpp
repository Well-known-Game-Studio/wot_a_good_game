// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableChest.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"
#include "UI/WotUWInventoryPanel.h"

// Sets default values
AWotOpenableChest::AWotOpenableChest() : AWotOpenable()
{
  BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
  BaseMesh->SetupAttachment(BaseSceneComp);

  LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
  LidMesh->SetupAttachment(BaseMesh);

	InventoryComp = CreateDefaultSubobject<UWotInventoryComponent>("InventoryComp");

  // chests cannot be closed by default, they can only be opened; this way they
  // indicate to the player that they have been interacted with
  bCanBeClosed = false;
}

void AWotOpenableChest::Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit)
{
  Super::Interact_Implementation(InstigatorPawn, Hit);
  LidMesh->SetRelativeRotation(FRotator(TargetPitch, 0, 0));
  if (!ensure(InventoryWidgetClass)) {
    UE_LOG(LogTemp, Error, TEXT("Missing required InventoryWidgetClass!"));
    return;
  }
  if (InventoryComp->Items.Num()) {
    // if we still have items in our inventory, show it
		UWotUWInventoryPanel* InventoryWidget = CreateWidget<UWotUWInventoryPanel>(GetWorld(), InventoryWidgetClass);
		InventoryWidget->SetInventory(InventoryComp, FText::FromName(InventoryPanelTitle));
		InventoryWidget->AddToViewport();
  }
}

void AWotOpenableChest::GetInteractionText_Implementation(APawn* InstigatorPawn, FHitResult Hit, FText& OutText)
{
  if (InventoryComp->Items.Num()) {
    OutText = FText::Format(FText::FromString("Open {0}"), FText::FromName(InventoryPanelTitle));
  } else {
    OutText = FText::Format(FText::FromString("Empty {0}"), FText::FromName(InventoryPanelTitle));
  }
}
