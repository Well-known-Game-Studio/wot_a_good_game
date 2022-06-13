#include "UI/WotUWInventoryPanel.h"
#include "UI/WotTextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

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
  if (Grid && ItemWidgetClass) {
    Grid->ClearChildren();

    for (int32 row = 0; row < Rows; ++row) {
      for (int32 col = 0; col < Columns; ++col) {
        UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), ItemWidgetClass);
        if (Widget) {
          UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(Widget, row, col);
          // GridSlot->SetColumn(col);
          // GridSlot->SetRow(row);
        }
      }
    }
  }
}

void UWotUWInventoryPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);
}
