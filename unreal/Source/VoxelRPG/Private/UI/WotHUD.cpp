#include "UI/WotHUD.h"
#include "UI/WotUserWidget.h"
#include <GameFramework/PlayerController.h>

void AWotHUD::ShowMainMenu()
{
  if (MainMenu) {
    // already showing
    return;
  }
  APlayerController* PC = Cast<APlayerController>(GetOwner());
  if (!PC || !MainMenuClass) {
    UE_LOG(LogTemp, Warning, TEXT("ShowMainMenu: missing owning PlayerController or MainMenuClass"));
    return;
  }
  MainMenu = CreateWidget<UWotUserWidget>(PC, MainMenuClass);
  if (MainMenu) {
    MainMenu->AddToViewport();
  }
}

void AWotHUD::HideMainMenu()
{
  if (MainMenu) {
    MainMenu->RemoveFromParent();
    MainMenu = nullptr;
  }
}
