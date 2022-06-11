#include "UI/WotUWPopup.h"
#include "UI/WotTextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UWotUWPopup::NativeConstruct()
{
  Super::NativeConstruct();
}

void UWotUWPopup::SetText(const FText& NewText)
{
  TextWidget->SetText(NewText);
}

void UWotUWPopup::SetColor(FLinearColor& NewColor)
{
  TextWidget->SetColorAndOpacity(NewColor);
}

void UWotUWPopup::PlayPopupAnimation()
{
  if (PopupAnim) {
    float StartAtTime = 0.0f;
    int NumLoops = 1;
    EUMGSequencePlayMode::Type PlayMode = EUMGSequencePlayMode::Type::Forward;
    float PlaybackSpeed = 1.0f;
    PlayAnimation(PopupAnim, StartAtTime, NumLoops, PlayMode, PlaybackSpeed);
  }
}

void UWotUWPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);

  if (!AttachTo.IsValid()) {
    return;
  }
  // now draw it in the right place (based on location of AttachTo actor)
  FVector Location = AttachTo->GetActorLocation();
  UWotUserWidget::SetPosition(Location);
}
