#include "UI/WotUWInventoryPanel.h"
#include "UI/WotUWItem.h"
#include "UI/WotTextBlock.h"
#include "WotCharacter.h"
#include "Components/WrapBox.h"
#include "WotInventoryComponent.h"

void UWotUWInventoryPanel::NativeConstruct()
{
  Super::NativeConstruct();
  LabelText = FText::FromString("Your Items");
  Setup();
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

void UWotUWInventoryPanel::SetInventory(UWotInventoryComponent* NewInventoryComp, FText NewLabelText)
{
  // Store the values
  InventoryComp = NewInventoryComp;
  LabelText = NewLabelText;
}

void UWotUWInventoryPanel::Close_Implementation()
{
  // remove ourselves from our parent (UI)
  RemoveFromParent();
  // Reset the mouse cursor and input mode
  APlayerController* PC = GetOwningPlayer();
  // convert to WotCharacter and inform it that the inventory panel is closed
  auto WotCharacter = Cast<AWotCharacter>(PC->GetPawn());
  if (WotCharacter) {
    WotCharacter->SetMenuActive(false);
  }

  PC->bShowMouseCursor = bControllerWasShowingCursor;
  // FInputModeGameOnly InputMode;
  // PC->SetInputMode(InputMode);
  // Make sure we don't update ourselves when the inventory updates
  InventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UWotUWInventoryPanel::UpdateInventory);
}

void UWotUWInventoryPanel::Setup()
{
  if (!InventoryComp) {
    return;
  }
  // Show mouse and focus this widget
  APlayerController* PC = GetOwningPlayer();
  bControllerWasShowingCursor = PC->bShowMouseCursor;
  PC->bShowMouseCursor = true;
  // FInputModeGameAndUI InputMode;
  // InputMode.SetWidgetToFocus(this);
  // PC->SetInputMode(InputMode);
  // Make sure we update ourselves when the inventory updates
  InventoryComp->OnInventoryUpdated.AddDynamic(this, &UWotUWInventoryPanel::UpdateInventory);
  // Now actually update the inventory
  UpdateInventory();
}

void UWotUWInventoryPanel::UpdateInventory()
{
  ItemBox->ClearChildren();
  if (!InventoryComp) {
    return;
  }
  for (auto& Item : InventoryComp->Items) {
    APawn* OwningPawn = GetOwningPlayerPawn();
    UWotUWItem* Widget = CreateWidget<UWotUWItem>(GetOwningPlayer(), ItemWidgetClass);
    Widget->Item = Item;
    Widget->bInOwningPlayerInventory = (InventoryComp->GetOwner() == OwningPawn);
    ItemBox->AddChildToWrapBox(Widget);
  }
}
