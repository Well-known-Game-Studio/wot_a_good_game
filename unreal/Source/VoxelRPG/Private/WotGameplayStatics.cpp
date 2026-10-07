#include "WotGameplayStatics.h"
#include "Engine/UserInterfaceSettings.h"
#include <Kismet/GameplayStatics.h>
#include "UI/WotHUD.h"

void UWotGameplayStatics::ShowMainMenu(const UObject* WorldContextObject, const int PlayerIndex)
{
  APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
  AWotHUD* HUD = PC ? PC->GetHUD<AWotHUD>() : nullptr;
  if (!HUD) {
    UE_LOG(LogTemp, Warning, TEXT("ShowMainMenu: no AWotHUD for player %d"), PlayerIndex);
    return;
  }
  HUD->ShowMainMenu();
}

void UWotGameplayStatics::HideMainMenu(const UObject* WorldContextObject, const int PlayerIndex)
{
  APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
  AWotHUD* HUD = PC ? PC->GetHUD<AWotHUD>() : nullptr;
  if (!HUD) {
    UE_LOG(LogTemp, Warning, TEXT("HideMainMenu: no AWotHUD for player %d"), PlayerIndex);
    return;
  }
  HUD->HideMainMenu();
}

void UWotGameplayStatics::SetUIScale( float CustomUIScale )
{
  UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>( UUserInterfaceSettings::StaticClass() );

  if ( UISettings ) {
    UISettings->ApplicationScale = CustomUIScale;
  }
}
