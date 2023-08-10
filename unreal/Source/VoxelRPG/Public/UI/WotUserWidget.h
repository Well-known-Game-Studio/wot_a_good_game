#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WotUserWidget.generated.h"

// We make the class abstract, as we don't want to create
// instances of this, instead we want to create instances
// of our UMG Blueprint subclass.
UCLASS(Abstract)
class VOXELRPG_API UWotUserWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable)
    virtual void SetDuration(float NewDuration);

    UFUNCTION(BlueprintCallable)
    virtual void SetOffset(const FVector& NewOffset);

protected:
    // Doing setup in the C++ constructor is not as
    // useful as using NativeConstruct.
	virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    virtual void SetPosition(const FVector& NewPosition);

    UPROPERTY( EditAnywhere, BlueprintReadWrite )
    float Duration{0.0f};

    UPROPERTY( EditAnywhere, BlueprintReadWrite )
    FVector Offset = FVector(0, 0, 0.0f);

    UPROPERTY()
	FTimerHandle TimerHandle_Remove;
    UFUNCTION()
	void Remove_TimeElapsed();
};
