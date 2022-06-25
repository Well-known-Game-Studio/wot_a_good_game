#pragma once

#include "CoreMinimal.h"
#include "WotArrowState.generated.h"

UENUM(BlueprintType)
enum class EWotArrowState : uint8 {
     Default    UMETA(DisplayName = "Default"),
     InBow      UMETA(DisplayName = "InBow"),
     InAir      UMETA(DisplayName = "InAir"),
     Unobtained UMETA(DisplayName = "Unobtained"),
};
