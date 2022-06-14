#include "UI/WotUWInventoryPanel.h"
#include "UI/WotTextBlock.h"
#include "Components/WrapBox.h"

void UWotUWInventoryPanel::NativeConstruct()
{
  Super::NativeConstruct();
}

// This is called every time that the widget is compiled,
// or a property is changed.
void UWotUWInventoryPanel::SynchronizeProperties()
{
  Super::SynchronizeProperties();

  // When first creating a Blueprint subclass of this class,
  // the widgets won't exist, so we must null check.
  if (Label) {
    Label->SetText(LabelText);
  }


  // Again, null checks are required
  if (ItemBox && ItemWidgetClass) {
    ItemBox->ClearChildren();

    int NumTestWidgets = 12;
    for (int i = 0; i < NumTestWidgets; ++i) {
      UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), ItemWidgetClass);
      if (Widget) {
        ItemBox->AddChildToWrapBox(Widget);
      }
    }
  }
}

void UWotUWInventoryPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);
}
