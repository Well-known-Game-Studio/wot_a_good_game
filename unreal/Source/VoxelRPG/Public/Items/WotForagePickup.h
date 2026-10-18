#pragma once

#include "CoreMinimal.h"
#include "Items/WotItemInteractableActor.h"
#include "WotForagePickup.generated.h"

class UWotItem;

/**
 * A placeable pickup for a world item (a mushroom in the woods, herbs by a stream): an interactable item actor that
 * builds its UWotItem from a class, so the town generator and level designers can drop it anywhere and pick the item
 * from a list. Shows the item's pickup mesh in the editor; the player picks it up with the interact key.
 */
UCLASS(Blueprintable)
class VOXELRPG_API AWotForagePickup : public AWotItemInteractableActor
{
	GENERATED_BODY()

public:
	AWotForagePickup();

	/** The item class to create (BP_Food_Mushroom_1 ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forage")
	TSubclassOf<UWotItem> ItemClass;

	/** How many the player gets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forage", meta = (ClampMin = 1))
	int32 Count = 1;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

protected:
	void BuildItem();
};
