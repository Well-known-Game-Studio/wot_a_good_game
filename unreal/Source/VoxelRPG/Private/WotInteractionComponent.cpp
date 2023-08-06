// Fill out your copyright notice in the Description page of Project Settings.

#include "WotInteractionComponent.h"
#include "WotCharacter.h"
#include "WotGameplayInterface.h"
#include "WotInventoryComponent.h"
#include "Items/WotItem.h"
#include "WotGameplayFunctionLibrary.h"

// For Debug:
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<bool> CVarDebugDrawInteraction(TEXT("wot.DebugDrawInteraction"), false, TEXT("Enable DebugDrawing for Interaction Component"), ECVF_Cheat);

// Sets default values for this component's properties
UWotInteractionComponent::UWotInteractionComponent()
{
}

void UWotInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UWotInteractionComponent::PrimaryInteract()
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	AActor* MyOwner = GetOwner();

	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	auto ForwardVector = MyOwner->GetActorForwardVector();
	auto OwnerLocation = MyOwner->GetActorLocation();
	auto OwnerRotation = MyOwner->GetActorRotation();

	FVector End = OwnerLocation + (ForwardVector * InteractionRange);

	AActor *ClosestActor = nullptr;
	UActorComponent *ClosestComponent = nullptr;
	FHitResult ClosestHit;
	bool didHit = UWotGameplayFunctionLibrary::GetClosestInteractibleInBox(MyOwner,
																		   InteractionBoxQueryHalfExtent,
																		   OwnerLocation,
																		   End,
																		   ClosestActor,
																		   ClosestComponent,
																		   ClosestHit);

	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

	if (ClosestActor) {
		APawn* MyPawn = Cast<APawn>(MyOwner);
		if (ClosestComponent) {
			IWotGameplayInterface::Execute_Interact(ClosestComponent, MyPawn, ClosestHit);
		} else {
			IWotGameplayInterface::Execute_Interact(ClosestActor, MyPawn, ClosestHit);
		}
		if (bDrawDebug) {
			UWotGameplayFunctionLibrary::DrawHitPointAndBounds(ClosestActor, ClosestHit);
		}
	}
}
