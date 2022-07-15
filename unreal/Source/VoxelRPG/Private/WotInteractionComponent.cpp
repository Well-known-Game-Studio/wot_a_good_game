// Fill out your copyright notice in the Description page of Project Settings.

#include "WotInteractionComponent.h"
#include "WotCharacter.h"
#include "WotGameplayInterface.h"
#include "WotInventoryComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Items/WotItem.h"
#include "WotGameplayFunctionLibrary.h"

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
		TArray<UClass*> BlueprintClasses;
		UWotGameplayFunctionLibrary::GetAllBlueprintSubclasses(UWotItem::StaticClass(), BlueprintClasses);
		// ItemClasses.Append(BlueprintClasses);
		for (auto Class : BlueprintClasses) {
			ItemClasses.AddUnique(Class);
		}
		TArray<UClass*> CppClasses;
		UWotGameplayFunctionLibrary::GetAllCppSubclasses(UWotItem::StaticClass(), CppClasses);
		// ItemClasses.Append(CppClasses);
		for (auto Class : CppClasses) {
			ItemClasses.AddUnique(Class);
		}
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
					UWotGameplayFunctionLibrary::DrawHitPointAndBounds(HitActor, Hit);
				}
				break;
			} else {
				// Handle foliage interaction here
				UInstancedStaticMeshComponent* ISMC = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
				if (ISMC) {
					UStaticMesh* StaticMesh = ISMC->GetStaticMesh();
					// loop through the foragable types and check if their mesh
					// matches this mesh; if so, create the
					UE_LOG(LogTemp, Warning, TEXT("Got ISMC (%s) with static mesh (%s)!"),
						   *GetNameSafe(ISMC),
						   *GetNameSafe(StaticMesh));
					for (auto& ItemClass : ItemClasses) {
						// See if the this is an interactible instance (e.g. if
						// there is an item whose mesh matches)
						UWotItem* DefaultObject = Cast<UWotItem>(ItemClass->GetDefaultObject());
						if (!DefaultObject) {
							UE_LOG(LogTemp, Error, TEXT("Invalid default object for class %s"), *ItemClass->GetFName().ToString());
							continue;
						}
						if (DefaultObject->PickupMesh == StaticMesh) {
							UE_LOG(LogTemp, Warning, TEXT("GOT ONE: %s (%s == %s), instance: %d"),
								   *GetNameSafe(DefaultObject),
								   *GetNameSafe(DefaultObject->PickupMesh),
								   *GetNameSafe(StaticMesh),
								   Hit.Item);
							// add one of these items to the player's inventory
							UWotInventoryComponent* InventoryComp = UWotInventoryComponent::GetInventory(MyOwner);
							if (InventoryComp) {
								UWotItem* NewItem = NewObject<UWotItem>(MyOwner, ItemClass);
								int32 NumAdded = InventoryComp->AddItem(NewItem);
								if (NumAdded == 0) {
									UE_LOG(LogTemp, Warning, TEXT("Didn't add to inventory, skipping!"));
									continue;
								}
							}
							// now remove the ISM instance
							ISMC->RemoveInstance(Hit.Item);
							// Show UI
							AWotCharacter* MyWotOwner = Cast<AWotCharacter>(MyOwner);
							if (MyWotOwner) {
								MyWotOwner->ShowPopupWidgetNumber(1, 1.0f);
							}
							// Draw debug info showing the hit / bounding box
							if (bDrawDebug) {
								UWotGameplayFunctionLibrary::DrawHitPointAndBounds(HitActor, Hit);
							}
							// now break out of this loop; since we're actually
							// in multiple loops, we will use a goto instead of
							// a break!
							goto found_hit;
						}
					}
				}
			}
		}
	}
found_hit:

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
