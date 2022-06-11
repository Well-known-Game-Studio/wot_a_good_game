#pragma once

#include "CoreMinimal.h"
#include "UI/WotUWPopup.h"
#include "WotUWPopupNumber.generated.h"

UCLASS()
class VOXELRPG_API UWotUWPopupNumber : public UWotUWPopup
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetNumber(int NewNumber);

protected:
    UPROPERTY(EditAnywhere)
    FLinearColor PositiveColor = FLinearColor(0.2f, 1.0f, 0.1f, 1.0f);

    UPROPERTY(EditAnywhere)
    FLinearColor NegativeColor = FLinearColor(1.0f, .2f, 0.1f, 1.0f);
};
