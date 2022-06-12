#pragma once

#include "CoreMinimal.h"
#include "UI/WotUserWidget.h"
#include "WotUWHealthBar.generated.h"

class UWotProgressBar;
class UWotTextBlock;

UCLASS()
class VOXELRPG_API UWotUWHealthBar : public UWotUserWidget
{
    GENERATED_BODY()

public:
    void SetAttachTo(AActor* InAttachTo) { AttachTo = InAttachTo; }
    void SetOffset(const FVector& NewOffset) { Offset = NewOffset; }

    UFUNCTION(BlueprintCallable)
    void SetHealth(float NewHealthStart, float NewHealthEnd, float HealthMax);

    UFUNCTION(BlueprintCallable)
    void SetFillColor(FLinearColor& NewFillColor);

    virtual void SetDuration(float NewDuration) override;

    UFUNCTION(BlueprintCallable)
    void PlayTextUpdateAnimation();

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	void NativeConstruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    TWeakObjectPtr<AActor> AttachTo;

    UPROPERTY( meta = ( BindWidget ) )
    UWotProgressBar* HealthBar;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* CurrentHealthLabel;

    UPROPERTY( meta = ( BindWidget ) )
    UWotTextBlock* MaxHealthLabel;

    UPROPERTY( Transient, meta = ( BindWidgetAnimOptional ) )
    UWidgetAnimation* TextUpdateAnim;

    UPROPERTY( EditAnywhere, BlueprintReadWrite )
    FVector Offset = FVector(0, 0, 100.0f);

    float HealthStart;
    float HealthCurrent;
    float HealthEnd;
    float HealthMax;
    float TimeRemaining{0};

    UFUNCTION()
    void UpdateHealth(float Interpolation);
};
