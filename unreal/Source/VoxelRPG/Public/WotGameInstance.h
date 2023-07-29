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
};
