// Fill out your copyright notice in the Description page of Project Settings.

#include "WotOpenableChest.h"
#include "Components/StaticMeshComponent.h"
#include "WotInventoryComponent.h"
#include "UI/WotUWInventoryPanel.h"

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
