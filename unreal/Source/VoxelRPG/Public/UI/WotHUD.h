#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WotHUD.generated.h"

class UWotUserWidget;

UCLASS()
class VOXELRPG_API AWotHUD : public AHUD
{
  GENERATED_BODY()

public:

  UFUNCTION(BlueprintCallable)
  void ShowMainMenu();

  UFUNCTION(BlueprintCallable)
  void HideMainMenu();

protected:

  UPROPERTY(EditDefaultsOnly)
  TSubclassOf<UWotUserWidget> MainMenuClass;

  UPROPERTY()
  UWotUserWidget* MainMenu;
};
