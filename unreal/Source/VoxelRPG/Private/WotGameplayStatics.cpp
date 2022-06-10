#include "WotGameplayStatics.h"
#include "Engine/UserInterfaceSettings.h"

void UWotGameplayStatics::SetUIScale( float CustomUIScale )
{
  UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>( UUserInterfaceSettings::StaticClass() );

  if ( UISettings ) {
    UISettings->ApplicationScale = CustomUIScale;
  }
}
