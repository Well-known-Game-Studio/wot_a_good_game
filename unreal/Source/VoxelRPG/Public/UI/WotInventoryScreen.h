#pragma once

#include "CoreMinimal.h"
#include "UI/WotUWInventoryPanel.h"
#include "WotInventoryScreen.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class UWotItem;

/** One slot of the inventory grid (icon, count badge, click to select). */
UCLASS()
class VOXELRPG_API UWotInventorySlot : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Index = -1;
	UPROPERTY() TObjectPtr<class UWotInventoryScreen> Screen;
	UPROPERTY() TObjectPtr<UButton> Button;
	UPROPERTY() TObjectPtr<UImage> Icon;
	UPROPERTY() TObjectPtr<UTextBlock> CountText;
	UFUNCTION() void OnClicked();
};

/**
 * Inventory screen built in C++: a grid of item slots with thumbnails and
 * counts, a details pane (name, description, weight) and Use / Drop /
 * Close buttons. Drop-in replacement for the WBP inventory panel: the
 * player Blueprint keeps calling SetInventory / Close.
 */
UCLASS()
class VOXELRPG_API UWotInventoryScreen : public UWotUWInventoryPanel
{
	GENERATED_BODY()

public:
	UWotInventoryScreen(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Screen")
	int32 Columns = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Screen")
	float SlotSize = 84.0f;

	UFUNCTION(BlueprintCallable, Category = "Inventory Screen")
	void SelectIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory Screen")
	void Refresh();

	virtual void Close_Implementation() override;
	virtual void UpdateInventory() override { Refresh(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION() void OnUseClicked();
	UFUNCTION() void OnDropClicked();
	UFUNCTION() void OnCloseClicked();

	UWotItem* SelectedItem() const;

	UPROPERTY() TObjectPtr<UTextBlock> TitleBlock;
	UPROPERTY() TObjectPtr<UTextBlock> WeightBlock;
	UPROPERTY() TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY() TObjectPtr<UImage> DetailIcon;
	UPROPERTY() TObjectPtr<UTextBlock> DetailName;
	UPROPERTY() TObjectPtr<UTextBlock> DetailCount;
	UPROPERTY() TObjectPtr<UTextBlock> DetailDescription;
	UPROPERTY() TObjectPtr<UButton> UseButton;
	UPROPERTY() TObjectPtr<UTextBlock> UseButtonText;
	UPROPERTY() TObjectPtr<UButton> DropButton;
	UPROPERTY() TArray<TObjectPtr<UWotInventorySlot>> Slots;

	int32 Selected = -1;
	bool bBound = false;
};
