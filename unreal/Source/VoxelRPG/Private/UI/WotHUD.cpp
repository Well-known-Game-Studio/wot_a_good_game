#include "UI/WotHUD.h"
#include "UI/WotUserWidget.h"
#include <GameFramework/PlayerController.h>

void AWotHUD::ShowMainMenu()
{
  APlayerController* PC = Cast<APlayerController>(GetOwner());
  MainMenu = CreateWidget<UWotUserWidget>(PC, MainMenuClass);

  MainMenu->AddToViewport();
}

void AWotHUD::HideMainMenu()
{
  if (MainMenu) {
    MainMenu->RemoveFromParent();
    MainMenu = nullptr;
  }
}
