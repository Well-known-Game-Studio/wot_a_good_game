#include "UI/WotUWPopup.h"
#include "UI/WotTextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UWotUWPopup::NativeConstruct()
{
  Super::NativeConstruct();
}

void UWotUWPopup::SetAttachTo(AActor* NewAttachTo)
{
  AttachTo = NewAttachTo;
  UpdatePosition();
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

void UWotUWPopup::UpdatePosition()
{
  if (AttachTo.IsValid()) {
    // now draw it in the right place (based on location of AttachTo actor)
    FVector Location = AttachTo->GetActorLocation();
    // apply the offset if it's set
    if (Offset != FVector::ZeroVector) {
      Location += Offset;
    }
    UWotUserWidget::SetPosition(Location);
  }
}

void UWotUWPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  UpdatePosition();
  Super::NativeTick(MyGeometry, InDeltaTime);
}
