#include "UI/WotUserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UWotUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// bind delegates, and set up default appearance
}

void UWotUserWidget::SetPosition(const FVector& NewPosition)
{
  APlayerController* PC = GetOwningPlayer();
  if (!PC) {
    return;
  }
  FVector2D ScreenPosition;
  UGameplayStatics::ProjectWorldToScreen(PC, NewPosition, ScreenPosition, false);
  float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
  SetRenderTranslation(ScreenPosition / ViewportScale);
}

void UWotUserWidget::SetDuration(float NewDuration)
{
  Duration = NewDuration;
  // now set a timer for destroying it
  GetWorld()->GetTimerManager().SetTimer(TimerHandle_Remove, this, &UWotUserWidget::Remove_TimeElapsed, Duration);
}

void UWotUserWidget::Remove_TimeElapsed()
{
  RemoveFromParent();
}
