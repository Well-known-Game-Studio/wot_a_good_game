#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWItem.generated.h"

class UImage;
class UWotTextBlock;

UCLASS()
class VOXELRPG_API UWotUWItem : public UWotUserWidget
{
    GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Details")
	FText LabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Details",
		meta=(BindWidget))
	UImage* Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Details",
		meta=(BindWidget))
	UWotTextBlock* Label = nullptr;

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
