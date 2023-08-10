// Fill out your copyright notice in the Description page of Project Settings.

#include "WotInteractionComponent.h"
#include "WotCharacter.h"
#include "WotInteractableInterface.h"
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

bool UWotInteractionComponent::IsInteractableInRange() const
{
	AActor* MyOwner = GetOwner();

	AActor *ClosestActor = nullptr;
	UActorComponent *ClosestComponent = nullptr;
	FHitResult ClosestHit;
	bool didHit = UWotGameplayFunctionLibrary::GetClosestInteractableInRange(MyOwner,
																			 InteractionRange,
																			 InteractionBoxQueryHalfExtent,
																			 ClosestActor,
																			 ClosestComponent,
																			 ClosestHit);

	return didHit;
}

bool UWotInteractionComponent::GetInteractableInRange(AActor *&OutActor, UActorComponent *&OutComponent, FHitResult &OutHit) const
{
	AActor* MyOwner = GetOwner();
	bool didHit = UWotGameplayFunctionLibrary::GetClosestInteractableInRange(MyOwner,
																			 InteractionRange,
																			 InteractionBoxQueryHalfExtent,
																			 OutActor,
																			 OutComponent,
																			 OutHit);
	return didHit;
}

void UWotInteractionComponent::PrimaryInteract()
{
	AActor* MyOwner = GetOwner();

	AActor *ClosestActor = nullptr;
	UActorComponent *ClosestComponent = nullptr;
	FHitResult ClosestHit;
	bool didHit = UWotGameplayFunctionLibrary::GetClosestInteractableInRange(MyOwner,
																			 InteractionRange,
																			 InteractionBoxQueryHalfExtent,
																			 ClosestActor,
																			 ClosestComponent,
																			 ClosestHit);

	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

	if (ClosestActor) {
		APawn* MyPawn = Cast<APawn>(MyOwner);
		if (ClosestComponent) {
			IWotInteractableInterface::Execute_Interact(ClosestComponent, MyPawn, ClosestHit);
		} else {
			IWotInteractableInterface::Execute_Interact(ClosestActor, MyPawn, ClosestHit);
		}
		if (bDrawDebug) {
			UWotGameplayFunctionLibrary::DrawHitPointAndBounds(ClosestActor, ClosestHit);
		}
	}
}
