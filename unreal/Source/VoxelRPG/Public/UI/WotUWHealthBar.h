#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotAttributeComponent.h"
#include "WotUWHealthBar.generated.h"

class UWotProgressBar;
class UWotTextBlock;

UCLASS()
class VOXELRPG_API UWotUWHealthBar : public UWotUserWidget
{
    GENERATED_BODY()

public:
    void SetOwnerAttributeComponent(UWotAttributeComponent* InAttributeComponent) { OwnerAttributeComp = InAttributeComponent; }

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    TWeakObjectPtr<UWotAttributeComponent> OwnerAttributeComp;

    UPROPERTY( meta = ( BindWidget ) )
    UWotProgressBar* HealthBar;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* CurrentHealthLabel;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* MaxHealthLabel;
};
