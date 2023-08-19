#include "UI/WotUserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UWotUserWidget::SetAttachTo(AActor* NewAttachTo)
{
  AttachTo = NewAttachTo;
  UpdatePosition();
}

void UWotUserWidget::SetOffset(const FVector& NewOffset)
{
  Offset = NewOffset;
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

void UWotUserWidget::UpdatePosition()
{
  if (AttachTo.IsValid()) {
    // now draw it in the right place (based on location of AttachTo actor)
    FVector Location = AttachTo->GetActorLocation() + Offset;
    SetPosition(Location);
  }
}

void UWotUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  UpdatePosition();
  Super::NativeTick(MyGeometry, InDeltaTime);
}
