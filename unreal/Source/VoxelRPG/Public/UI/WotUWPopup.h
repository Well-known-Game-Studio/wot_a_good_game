#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWPopup.generated.h"

class UWotTextBlock;

UCLASS()
class VOXELRPG_API UWotUWPopup : public UWotUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetText(const FText& NexText);

    UFUNCTION(BlueprintCallable)
    void SetColor(FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable)
    void PlayPopupAnimation();

protected:
    UPROPERTY( Transient, meta = ( BindWidgetAnimOptional ) )
    UWidgetAnimation* PopupAnim;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* TextWidget;
};
