#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWInventoryPanel.generated.h"

class UWotInventoryComponent;
class UWotTextBlock;
class UWrapBox;
class UWotUWItem;

UCLASS()
class VOXELRPG_API UWotUWInventoryPanel : public UWotUserWidget
{
    GENERATED_BODY()

public:
	virtual void SynchronizeProperties() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory Panel",  meta = (ExposeOnSpawn=true))
	FText LabelText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory Panel",  meta = (ExposeOnSpawn=true))
    UWotInventoryComponent* InventoryComp;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	TSubclassOf<UWotUWItem> ItemWidgetClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UWotTextBlock* Label = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UWrapBox* ItemBox = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Inventory Panel")
    void SetInventory(UWotInventoryComponent* NewInventoryComp, FText NewLabelText);

    UFUNCTION(BlueprintCallable, Category = "Inventory Panel")
    void UpdateInventory();

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
