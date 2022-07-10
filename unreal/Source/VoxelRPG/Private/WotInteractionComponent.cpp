// Fill out your copyright notice in the Description page of Project Settings.

#include "WotInteractionComponent.h"
#include "WotGameplayInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Items/WotItem.h"

// For Debug:
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<bool> CVarDebugDrawInteraction(TEXT("wot.DebugDrawInteraction"), false, TEXT("Enable DebugDrawing for Interaction Component"), ECVF_Cheat);

// For use with turning foliage into foragable items
static TArray<UClass*> ItemClasses;

// Sets default values for this component's properties
UWotInteractionComponent::UWotInteractionComponent()
{
}

void UWotInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ItemClasses.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("Getting all subclasses of UWotItem"));
		bool bRecursive = true;
		GetDerivedClasses(UWotItem::StaticClass(), ItemClasses, bRecursive);
		UE_LOG(LogTemp, Warning, TEXT("Got %d subclasses of UWotItem"), ItemClasses.Num());
	}
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

	for (auto Hit : Hits) {
		AActor* HitActor = Hit.GetActor();
		if (HitActor) {
			// If it imiplements the GameplayInterface (Interact)
			if (HitActor->Implements<UWotGameplayInterface>()) {
				APawn* MyPawn = Cast<APawn>(MyOwner);
				IWotGameplayInterface::Execute_Interact(HitActor, MyPawn);
				if (bDrawDebug) {
					FVector HitActorLocation;
					FVector HitBoxExtent;
					HitActor->GetActorBounds(false, HitActorLocation, HitBoxExtent, false);
					// draw a box around what was hit
					DrawDebugBox(GetWorld(), HitActorLocation, HitBoxExtent, HitActor->GetActorRotation().Quaternion(), FColor::Green, false, 2.0f, 0, 2.0f);
					// draw a point for the hit location itself
					DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10, FColor::Red, false, 2.0f, 100);
				}
				break;
			} else {
				// TODO: implement the foliage interaction here
				UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(HitActor->GetComponentByClass(UInstancedStaticMeshComponent::StaticClass()));
				if (ISMC) {
					UStaticMesh* StaticMesh = ISMC->GetStaticMesh();
					// loop through the foragable types and check if their mesh
					// matches this mesh; if so, create the
					UE_LOG(LogTemp, Warning, TEXT("Got ISMC (%s) with static mesh (%s)!"),
						   *GetNameSafe(ISMC),
						   *GetNameSafe(StaticMesh));
					for (auto& ItemClass : ItemClasses) {
						// UWotItem* ItemClass = Cast<UWotItem>(ItemClass);
						UWotItem* DefaultObject = Cast<UWotItem>(ItemClass->GetDefaultObject());
						if (DefaultObject->PickupMesh == StaticMesh) {
							UE_LOG(LogTemp, Warning, TEXT("GOT ONE: %s (%s == %s)"),
								   *GetNameSafe(DefaultObject),
								   *GetNameSafe(DefaultObject->PickupMesh),
								   *GetNameSafe(StaticMesh));
							if (bDrawDebug) {
								FVector HitActorLocation;
								FVector HitBoxExtent;
								HitActor->GetActorBounds(false, HitActorLocation, HitBoxExtent, false);
								// draw a box around what was hit
								DrawDebugBox(GetWorld(), HitActorLocation, HitBoxExtent, HitActor->GetActorRotation().Quaternion(), FColor::Green, false, 2.0f, 0, 2.0f);
								// draw a point for the hit location itself
								DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10, FColor::Red, false, 2.0f, 100);
							}
						}
					}
				}
			}
		}
	}

	if (bDrawDebug) {
		FColor LineColor = bBlockingHit ? FColor::Green : FColor::Red;
		// Draw a line for the vector from the owner to the end of the sweep
		DrawDebugLine(GetWorld(), OwnerLocation, End, LineColor, false, 5.0f, 0, 10.0f);
		// Draw the box representing how we swept
		FVector SweepExtent(InteractionRange / 2, HalfExtent.Y, HalfExtent.Z);
		DrawDebugBox(GetWorld(),
					 OwnerLocation + (ForwardVector * InteractionRange) / 2,
					 SweepExtent,
					 OwnerRotation.Quaternion(),
					 LineColor, false, 2.0f, 0, 2.0f);
	}
}
