// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableChest.h"
#include "Components/StaticMeshComponent.h"
#include "WotCharacter.h"
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

void AWotOpenableChest::SetHighlightEnabled(int HighlightValue, bool Enabled)
{
  BaseMesh->SetRenderCustomDepth(Enabled);
  LidMesh->SetRenderCustomDepth(Enabled);
  BaseMesh->SetCustomDepthStencilValue(HighlightValue);
  LidMesh->SetCustomDepthStencilValue(HighlightValue);
}

void AWotOpenableChest::Interact_Implementation(APawn* InstigatorPawn, FHitResult Hit)
{
  Super::Interact_Implementation(InstigatorPawn, Hit);
  // ensure we have a valid instigator pawn
  AWotCharacter* WotCharacter = Cast<AWotCharacter>(InstigatorPawn);
  if (!ensure(WotCharacter)) {
    UE_LOG(LogTemp, Error, TEXT("InstigatorPawn is not a WotCharacter!"));
    return;
  }

  // If the character can't open the menu, then we don't do anything
  if (!WotCharacter->CanOpenInventory()) {
    UE_LOG(LogTemp, Warning, TEXT("Character cannot open inventory!"));
    return;
  }

  // set the lid to be open
  LidMesh->SetRelativeRotation(FRotator(TargetPitch, 0, 0));
  if (!ensure(InventoryWidgetClass)) {
    UE_LOG(LogTemp, Error, TEXT("Missing required InventoryWidgetClass!"));
    return;
  }

  // if we still have items in our inventory, show it
  if (InventoryComp->Items.Num()) {
		UWotUWInventoryPanel* InventoryWidget;
    WotCharacter->ShowInventoryWidget(InventoryWidget);
		InventoryWidget->SetInventory(InventoryComp, FText::FromName(InventoryPanelTitle));
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
