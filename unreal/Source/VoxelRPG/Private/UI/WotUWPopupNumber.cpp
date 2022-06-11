#include "UI/WotUWPopupNumber.h"

void UWotUWPopupNumber::SetNumber(int NewNumber)
{
  FNumberFormattingOptions Opts;
  Opts.AlwaysSign = true;
  Opts.SetMaximumFractionalDigits(0);
  FText PopupText = FText::AsNumber(NewNumber, &Opts);
  SetText(PopupText);
  if (NewNumber < 0) {
    SetColor(NegativeColor);
  } else {
    SetColor(PositiveColor);
  }
}
