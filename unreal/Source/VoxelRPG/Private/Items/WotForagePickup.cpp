#include "Items/WotForagePickup.h"
#include "Components/StaticMeshComponent.h"
#include "Items/WotItem.h"

AWotForagePickup::AWotForagePickup()
{
	// something to look at and trace against, never an obstacle for the navmesh
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
}

void AWotForagePickup::BuildItem()
{
	if (!ItemClass) {
		return;
	}
	if (!Item || Item->GetClass() != ItemClass) {
		UWotItem* NewItem = NewObject<UWotItem>(this, ItemClass);
		NewItem->Count = FMath::Max(1, Count);
		SetItem(NewItem);
	} else {
		Item->Count = FMath::Max(1, Count);
	}
}

void AWotForagePickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildItem();   // the editor shows the item's pickup mesh as soon as ItemClass is set
}

void AWotForagePickup::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	BuildItem();
}
