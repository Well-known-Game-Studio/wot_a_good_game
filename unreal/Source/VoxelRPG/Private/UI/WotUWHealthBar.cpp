#include "UI/WotUWHealthBar.h"
#include "UI/WotProgressBar.h"
#include "UI/WotTextBlock.h"

void UWotUWHealthBar::NativeConstruct()
{
  Super::NativeConstruct();
}

void UWotUWHealthBar::SetFillColor(FLinearColor& NewFillColor)
{
  HealthBar->SetFillColorAndOpacity(NewFillColor);
}

void UWotUWHealthBar::SetDuration(float NewDuration) {
  TimeRemaining = Duration;
  Super::SetDuration(NewDuration);
}

void UWotUWHealthBar::SetHealth(float NewHealthStart, float NewHealthEnd, float NewHealthMax)
{
  HealthStart = NewHealthStart;
  HealthCurrent = HealthStart;
  HealthEnd = NewHealthEnd;
  HealthMax = NewHealthMax;
  // Set the health text and progress
  UpdateHealth(1.0f);
}

void UWotUWHealthBar::PlayTextUpdateAnimation()
{
  if (TextUpdateAnim) {
    float StartAtTime = 0.0f;
    int NumLoops = 1;
    EUMGSequencePlayMode::Type PlayMode = EUMGSequencePlayMode::Type::Forward;
    float PlaybackSpeed = 1.0f;
    PlayAnimation(TextUpdateAnim, StartAtTime, NumLoops, PlayMode, PlaybackSpeed);
  }
}

void UWotUWHealthBar::UpdateHealth(float Interpolation)
{
  // Lerp current health between Start/End based on Interpolation
  HealthCurrent = (HealthStart - HealthEnd) * Interpolation + HealthEnd;
  HealthBar->SetPercent(HealthCurrent / HealthMax);
  FNumberFormattingOptions Opts;
  Opts.SetMaximumFractionalDigits(0);
  CurrentHealthLabel->SetText(FText::AsNumber(HealthCurrent, &Opts));
  MaxHealthLabel->SetText(FText::AsNumber(HealthMax, &Opts));
}

void UWotUWHealthBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);

  if (!AttachTo.IsValid()) {
    return;
  }
  // Use the duration as the interpolation
  TimeRemaining = std::max(TimeRemaining - InDeltaTime, 0.0f);
  float Interpolation = 0;
  if (Duration > 0) {
    Interpolation = TimeRemaining / Duration;
  }
  // Lerp the Health
  UpdateHealth(Interpolation);
  // now draw it in the right place (based on location of AttachTo actor)
  FVector Location = AttachTo->GetActorLocation() + Offset;
  SetPosition(Location);
}
