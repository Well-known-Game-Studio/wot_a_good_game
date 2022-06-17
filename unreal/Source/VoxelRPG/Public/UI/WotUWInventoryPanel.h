#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWInventoryPanel.generated.h"

class UWotTextBlock;
class UWrapBox;

UCLASS()
class VOXELRPG_API UWotUWInventoryPanel : public UWotUserWidget
{
    GENERATED_BODY()

public:
	virtual void SynchronizeProperties() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory Panel",  meta = (ExposeOnSpawn=true))
	FText LabelText;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	TSubclassOf<UWotUserWidget> ItemWidgetClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UWotTextBlock* Label = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UWrapBox* ItemBox = nullptr;

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
