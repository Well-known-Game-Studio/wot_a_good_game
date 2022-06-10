#include "UI/WotUWHealthBar.h"
#include "UI/WotProgressBar.h"
#include "UI/WotTextBlock.h"
#include "WotAttributeComponent.h"

void UWotUWHealthBar::NativeConstruct()
{
  Super::NativeConstruct();
}

void UWotUWHealthBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);

  if (!OwnerAttributeComp.IsValid()) {
    return;
  }

  float CurrentHealth = OwnerAttributeComp->GetHealth();
  float MaxHealth = OwnerAttributeComp->GetHealthMax();
  HealthBar->SetPercent(CurrentHealth / MaxHealth);
  FNumberFormattingOptions Opts;
  Opts.SetMaximumFractionalDigits(0);
  CurrentHealthLabel->SetText(FText::AsNumber(CurrentHealth, &Opts));
  MaxHealthLabel->SetText(FText::AsNumber(MaxHealth, &Opts));
}
