#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWInventoryPanel.generated.h"

class UWotTextBlock;
class UUniformGridPanel;

UCLASS()
class VOXELRPG_API UWotUWInventoryPanel : public UWotUserWidget
{
    GENERATED_BODY()

public:
	virtual void SynchronizeProperties() override;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	FText LabelText;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	TSubclassOf<UWotUserWidget> ItemWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, Category = "Inventory Panel")
	int32 Rows = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UWotTextBlock* Label = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Panel",
		meta=(BindWidget))
	UUniformGridPanel* Grid = nullptr;

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
