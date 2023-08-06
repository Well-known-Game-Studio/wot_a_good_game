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

	TArray<FHitResult> Hits;

	FCollisionShape Shape;
	Chaos::TVector<float, 3> HalfExtent = InteractionBoxQueryHalfExtent;
    FVector Extent = FVector(HalfExtent.X, HalfExtent.Y, HalfExtent.Z);
	Shape.SetBox(HalfExtent);

	bool bBlockingHit = GetWorld()->SweepMultiByObjectType(Hits,
														   OwnerLocation,
														   End,
														   FQuat::Identity,
														   ObjectQueryParams,
														   Shape);

	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

	// find the closest interactable actor or component from the list
	AActor* ClosestActor = nullptr;
	UActorComponent* ClosestComponent = nullptr;
	float ClosestDistance = InteractionRange;
	FHitResult ClosestHit;

	for (auto Hit : Hits) {
		AActor* HitActor = Hit.GetActor();
		if (HitActor) {
			// get the distance to the hit location
			float Distance = FVector::Dist(Hit.Location, OwnerLocation);
			// If it imiplements the GameplayInterface (Interact)
			if (HitActor->Implements<UWotGameplayInterface>()) {
				if (Distance < ClosestDistance) {
					ClosestActor = HitActor;
					ClosestDistance = Distance;
					ClosestHit = Hit;
					// unset the closest component, since we found an actor
					ClosestComponent = nullptr;
				}
			} else {
				// The actor doesn't implement the GameplayInterface, so try to
				// get a component that does
				auto components = HitActor->GetComponentsByInterface(UWotGameplayInterface::StaticClass());
				// TODO: how to handle multiple components that implement the interface?
				if (components.Num() > 0) {
					auto component = components[0];
					if (Distance < ClosestDistance) {
						// we also have to set the closest actor to the hit actor
						// so that we can draw the debug lines
						ClosestActor = HitActor;
						ClosestComponent = component;
						ClosestDistance = Distance;
						ClosestHit = Hit;
					}
				}
			}
		}
	}
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
