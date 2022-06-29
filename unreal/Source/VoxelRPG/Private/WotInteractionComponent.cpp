// Fill out your copyright notice in the Description page of Project Settings.

#include "WotInteractionComponent.h"
#include "WotGameplayInterface.h"

// For Debug:
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<bool> CVarDebugDrawInteraction(TEXT("wot.DebugDrawInteraction"), true, TEXT("Enable DebugDrawing for Interaction Component"), ECVF_Cheat);

// Sets default values for this component's properties
UWotInteractionComponent::UWotInteractionComponent()
{
}

void UWotInteractionComponent::PrimaryInteract()
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	AActor* MyOwner = GetOwner();

	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	auto ForwardVector = MyOwner->GetActorForwardVector();

	FVector End = EyeLocation + (ForwardVector * InteractionRange);

	TArray<FHitResult> Hits;

	FCollisionShape Shape;
	Chaos::TVector<float, 3> HalfExtent = Chaos::TVector<float, 3>(32.0f, 32.0f, 100.0f);
    FVector Extent = FVector(HalfExtent.X, HalfExtent.Y, HalfExtent.Z);
	Shape.SetBox(HalfExtent);

	bool bBlockingHit = GetWorld()->SweepMultiByObjectType(Hits,
														   EyeLocation,
														   End,
														   FQuat::Identity,
														   ObjectQueryParams,
														   Shape);

	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

	for (auto Hit : Hits) {
		AActor* HitActor = Hit.GetActor();
		if (HitActor) {
			if (HitActor->Implements<UWotGameplayInterface>()) {
				APawn* MyPawn = Cast<APawn>(MyOwner);
				IWotGameplayInterface::Execute_Interact(HitActor, MyPawn);
				if (bDrawDebug) {
					DrawDebugBox(GetWorld(), Hit.ImpactPoint, Extent, EyeRotation.Quaternion(), FColor::Green, false, 2.0f, 0, 2.0f);
				}
				break;
			} else {
				// TODO: get all components and see if they implement it
			}
		}
		if (bDrawDebug) {
			DrawDebugBox(GetWorld(), Hit.ImpactPoint, Extent, EyeRotation.Quaternion(), FColor::Red, false, 2.0f, 0, 2.0f);
		}
	}

	if (bDrawDebug) {
		FColor LineColor = bBlockingHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), EyeLocation, End, LineColor, false, 5.0f, 0, 10.0f);
	}
}
