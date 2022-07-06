#include "UI/WotUWItem.h"
#include "UI/WotTextBlock.h"
#include "Components/Image.h"
#include "WotGameplayFunctionLibrary.h"
#include "Items/WotItem.h"

void UWotUWItem::NativeConstruct()
{
  Super::NativeConstruct();
  SetItem(Item, bInOwningPlayerInventory);
}

void UWotUWItem::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);
  // Since the UseActionText can change (as it is used), we need to make sure to
  // update the use tooltip text in the native tick.
  if (bInOwningPlayerInventory) {
    UseTooltipText = Item->UseActionText;
  } else {
    // TODO: find better way of setting this (for translations and such?)
    UseTooltipText = FText::FromString("Take");
  }
}

void UWotUWItem::SetItem(UWotItem* NewItem, bool NewInOwningPlayerInventory)
{
  // Store the values
  Item = NewItem;
  bInOwningPlayerInventory = NewInOwningPlayerInventory;
  if (!Item) {
    return;
  }
  // set the texture for the widget {button
  Image->SetBrushFromTexture(Item->Thumbnail);
  NameLabel->SetText(Item->ItemDisplayName);
  CountLabel->SetText(UWotGameplayFunctionLibrary::GetIntAsText(Item->Count));
}
