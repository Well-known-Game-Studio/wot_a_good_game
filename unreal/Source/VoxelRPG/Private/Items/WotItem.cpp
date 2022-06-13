#include "Items/WotItem.h"

UWotItem::UWotItem()
{
  UseActionText = FText::FromString("Use");
  ItemDisplayName = FText::FromString("Item");
  ItemDescription = FText::FromString("Description");
  Weight = 1.0f;
}
