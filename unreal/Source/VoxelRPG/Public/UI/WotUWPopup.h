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
    void SetAttachTo(AActor* InAttachTo) { AttachTo = InAttachTo; }

    UFUNCTION(BlueprintCallable)
    void SetText(const FText& NexText);

    UFUNCTION(BlueprintCallable)
    void SetColor(FLinearColor& NewColor);

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    TWeakObjectPtr<AActor> AttachTo;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* TextWidget;
};
