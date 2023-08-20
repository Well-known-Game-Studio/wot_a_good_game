#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WotGameInstance.generated.h"

UCLASS()
class VOXELRPG_API UWotGameInstance : public UGameInstance
{
  GENERATED_BODY()

public:

    // Store the FName of the last portal the player used
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
    FName LastPortalName;

    // Store the Current time of day in seconds
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Time")
    float CurrentTimeOfDayInSeconds;

    // Sets the current time of day in seconds to be the
    // CurrentTimeOfDayInSeconds variable
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
    void LoadTimeOfDay();

    // Gets the time of day in seconds and stores it in the
    // CurrentTimeOfDayInSeconds variable
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
    void SaveTimeOfDay();

    // Checks if it is daytime and returns true if it is
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Time")
    bool IsDaytime();
};
