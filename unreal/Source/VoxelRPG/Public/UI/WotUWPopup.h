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
    void SetAttachTo(AActor* InAttachTo);

    UFUNCTION(BlueprintCallable)
    void SetText(const FText& NexText);

    UFUNCTION(BlueprintCallable)
    void SetColor(FLinearColor& NewColor);

    UFUNCTION(BlueprintCallable)
    void PlayPopupAnimation();

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void UpdatePosition();

    TWeakObjectPtr<AActor> AttachTo;

    UPROPERTY( Transient, meta = ( BindWidgetAnimOptional ) )
    UWidgetAnimation* PopupAnim;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* TextWidget;
};
