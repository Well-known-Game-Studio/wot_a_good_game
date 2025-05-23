#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EngineMinimal.h"
#include "WotGameplayStatics.generated.h"

UCLASS()
class UWotGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION( BlueprintCallable, Category = "User Interface" )
	static void ShowMainMenu(const UObject* WorldContextObject, const int PlayerIndex=0);

	UFUNCTION( BlueprintCallable, Category = "User Interface" )
	static void HideMainMenu(const UObject* WorldContextObject, const int PlayerIndex=0);

	// Custom UI scale 1.0f == 100%, 2.0f == 200%, 0.5f == 50% etc.
	UFUNCTION( BlueprintCallable, Category = "User Interface" )
	static void SetUIScale( float CustomUIScale );
};
