#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWItem.generated.h"

class UImage;
class UWotTextBlock;
class UWotItem;

UCLASS()
class VOXELRPG_API UWotUWItem : public UWotUserWidget
{
    GENERATED_BODY()

public:
    // The actual item this widget represents
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Details",  meta = (ExposeOnSpawn=true))
	UWotItem* Item;

    // True of the item is in the inventory of the player viewing this widget.
    // Controls whether the item can be dropped and whether the use text shows
    // up as "Take" or not.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Details",  meta = (ExposeOnSpawn=true))
	bool bInOwningPlayerInventory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Details")
	FText LabelText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Details")
	FText UseTooltipText;

	UPROPERTY(BlueprintReadOnly, Category = "Details",
		meta=(BindWidget))
	UImage* Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Details",
		meta=(BindWidget))
	UWotTextBlock* NameLabel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Details",
		meta=(BindWidget))
	UWotTextBlock* CountLabel = nullptr;

    void SetItem(UWotItem* NewItem, bool NewInOwningPlayerInventory);

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
