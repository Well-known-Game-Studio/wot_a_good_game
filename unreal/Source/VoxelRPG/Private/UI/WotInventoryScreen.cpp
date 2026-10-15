#include "UI/WotInventoryScreen.h"
#include "WotCharacter.h"
#include "UI/WotMenuWidget.h"
#include "WotInventoryComponent.h"
#include "Items/WotItem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

static FSlateBrush WotSlotBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Width)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Fill);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(8, 8, 8, 8), FSlateColor(Outline), Width);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	return Brush;
}

void UWotInventorySlot::OnClicked()
{
	if (Screen) {
		Screen->SelectIndex(Index);
	}
}

UWotInventoryScreen::UWotInventoryScreen(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LabelText = NSLOCTEXT("Wot", "Inventory", "Inventory");
}

TSharedRef<SWidget> UWotInventoryScreen::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("InvRoot"));
		WidgetTree->RootWidget = Root;
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrushColor(WotUI::Backdrop);
		if (UOverlaySlot* S = Root->AddChildToOverlay(Backdrop)) {
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvSize"));
		Size->SetWidthOverride(SlotSize * Columns + 380.0f);
		if (UOverlaySlot* S = Root->AddChildToOverlay(Size)) {
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetVerticalAlignment(VAlign_Center);
		}
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvPanel"));
		Panel->SetBrush(WotSlotBrush(WotUI::Panel, WotUI::PanelLine, 2.0f));
		Panel->SetPadding(FMargin(24.0f, 18.0f));
		Size->AddChild(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Column);

		// header: title + weight
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		TitleBlock = UWotMenuWidget::MakeText(WidgetTree, LabelText, 26, WotUI::Title, true);
		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(TitleBlock)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		WeightBlock = UWotMenuWidget::MakeText(WidgetTree, FText::GetEmpty(), 14, WotUI::Muted);
		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(WeightBlock)) {
			S->SetVerticalAlignment(VAlign_Bottom);
		}
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Header)) {
			S->SetPadding(FMargin(0, 0, 0, 12.0f));
		}

		// body: grid | details
		UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Body);
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
		Grid->SetSlotPadding(FMargin(4.0f));
		Grid->SetMinDesiredSlotWidth(SlotSize);
		Grid->SetMinDesiredSlotHeight(SlotSize);
		Scroll->AddChild(Grid);
		USizeBox* GridSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		GridSize->SetWidthOverride((SlotSize + 8.0f) * Columns);
		GridSize->SetMaxDesiredHeight((SlotSize + 8.0f) * 4);
		GridSize->AddChild(Scroll);
		Body->AddChildToHorizontalBox(GridSize);

		UBorder* Details = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Details"));
		Details->SetBrush(WotSlotBrush(WotUI::ButtonNormal, WotUI::PanelLine * FLinearColor(1, 1, 1, 0.5f), 1.0f));
		Details->SetPadding(FMargin(14.0f));
		if (UHorizontalBoxSlot* S = Body->AddChildToHorizontalBox(Details)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetPadding(FMargin(14.0f, 0, 0, 0));
		}
		UVerticalBox* DetailCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Details->SetContent(DetailCol);
		DetailIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DetailIcon->SetDesiredSizeOverride(FVector2D(96.0f, 96.0f));
		if (UVerticalBoxSlot* S = DetailCol->AddChildToVerticalBox(DetailIcon)) {
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0, 0, 0, 8.0f));
		}
		DetailName = UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "SelectItem", "Select an item"), 19, WotUI::Title, true);
		DetailName->SetJustification(ETextJustify::Center);
		DetailCol->AddChildToVerticalBox(DetailName);
		DetailCount = UWotMenuWidget::MakeText(WidgetTree, FText::GetEmpty(), 13, WotUI::Muted);
		DetailCount->SetJustification(ETextJustify::Center);
		DetailCol->AddChildToVerticalBox(DetailCount);
		DetailDescription = UWotMenuWidget::MakeText(WidgetTree, FText::GetEmpty(), 14, WotUI::Text);
		DetailDescription->SetAutoWrapText(true);
		if (UVerticalBoxSlot* S = DetailCol->AddChildToVerticalBox(DetailDescription)) {
			S->SetPadding(FMargin(0, 10.0f));
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		DetailCol->AddChildToVerticalBox(Actions);
		UseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UWotMenuWidget::StyleButton(UseButton, WotUI::Selected);
		UseButtonText = UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "Use", "Use"), 15, WotUI::Text);
		UseButton->AddChild(UseButtonText);
		UseButton->OnClicked.AddDynamic(this, &UWotInventoryScreen::OnUseClicked);
		if (UHorizontalBoxSlot* S = Actions->AddChildToHorizontalBox(UseButton)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetPadding(FMargin(0, 0, 6.0f, 0));
		}
		DropButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UWotMenuWidget::StyleButton(DropButton, WotUI::Danger);
		DropButton->AddChild(UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "Drop", "Drop"), 15, WotUI::Text));
		DropButton->OnClicked.AddDynamic(this, &UWotInventoryScreen::OnDropClicked);
		if (UHorizontalBoxSlot* S = Actions->AddChildToHorizontalBox(DropButton)) {
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		// footer
		UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UWotMenuWidget::StyleButton(CloseBtn);
		CloseBtn->AddChild(UWotMenuWidget::MakeText(WidgetTree, NSLOCTEXT("Wot", "CloseInv", "Close   (I / Esc)"), 15, WotUI::Text));
		CloseBtn->OnClicked.AddDynamic(this, &UWotInventoryScreen::OnCloseClicked);
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(CloseBtn)) {
			S->SetPadding(FMargin(0, 14.0f, 0, 0));
			S->SetHorizontalAlignment(HAlign_Right);
		}
	}
	return UUserWidget::RebuildWidget();
}

void UWotInventoryScreen::NativeConstruct()
{
	// deliberately skips UWotUWInventoryPanel::NativeConstruct, which expects BindWidget parts of the UMG asset
	UUserWidget::NativeConstruct();
	SetIsFocusable(true);
	if (APlayerController* PC = GetOwningPlayer()) {
		bControllerWasShowingCursor = PC->ShouldShowMouseCursor();
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	SetKeyboardFocus();
	Refresh();
}

void UWotInventoryScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	UUserWidget::NativeTick(MyGeometry, InDeltaTime);
	if (!bBound && InventoryComp) {
		bBound = true; // the base class binds OnInventoryUpdated -> UpdateInventory -> Refresh
		Refresh();
	}
}

FReply UWotInventoryScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::I || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right) {
		Close();
		return FReply::Handled();
	}
	if (Slots.Num() > 0) {
		int32 Next = Selected;
		if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right) Next = (Selected + 1) % Slots.Num();
		else if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left) Next = (Selected - 1 + Slots.Num()) % Slots.Num();
		else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) Next = FMath::Min(Selected + Columns, Slots.Num() - 1);
		else if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) Next = FMath::Max(Selected - Columns, 0);
		else if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom) { OnUseClicked(); return FReply::Handled(); }
		if (Next != Selected) {
			SelectIndex(Next);
			return FReply::Handled();
		}
	}
	return UUserWidget::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UWotItem* UWotInventoryScreen::SelectedItem() const
{
	if (!InventoryComp || !InventoryComp->Items.IsValidIndex(Selected)) {
		return nullptr;
	}
	return InventoryComp->Items[Selected];
}

void UWotInventoryScreen::Refresh()
{
	if (!Grid || !WidgetTree) {
		return;
	}
	Grid->ClearChildren();
	Slots.Reset();
	if (TitleBlock) {
		TitleBlock->SetText(LabelText.IsEmpty() ? NSLOCTEXT("Wot", "Inventory", "Inventory") : LabelText);
	}
	float Weight = 0.0f;
	int32 Count = 0;
	const int32 Num = InventoryComp ? InventoryComp->Items.Num() : 0;
	const int32 Cells = FMath::Max(Num, Columns * 3);
	for (int32 i = 0; i < Cells; ++i) {
		UWotItem* Item = (InventoryComp && InventoryComp->Items.IsValidIndex(i)) ? InventoryComp->Items[i] : nullptr;
		UWotInventorySlot* S = NewObject<UWotInventorySlot>(this);
		S->Index = i;
		S->Screen = this;
		S->Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UWotMenuWidget::StyleButton(S->Button, i == Selected ? WotUI::Selected : (Item ? WotUI::ButtonNormal : WotUI::ButtonNormal * FLinearColor(1, 1, 1, 0.45f)));
		S->Button->SetIsEnabled(Item != nullptr);
		USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Cell->SetWidthOverride(SlotSize);
		Cell->SetHeightOverride(SlotSize);
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Cell->AddChild(Stack);
		S->Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Item && Item->Thumbnail) {
			S->Icon->SetBrushFromTexture(Item->Thumbnail, false);
		} else {
			S->Icon->SetColorAndOpacity(FLinearColor(1, 1, 1, Item ? 0.25f : 0.0f));
		}
		if (UOverlaySlot* OS = Stack->AddChildToOverlay(S->Icon)) {
			OS->SetHorizontalAlignment(HAlign_Fill);
			OS->SetVerticalAlignment(VAlign_Fill);
			OS->SetPadding(FMargin(6.0f));
		}
		if (Item) {
			if (!Item->Thumbnail) {
				UTextBlock* Initial = UWotMenuWidget::MakeText(WidgetTree, FText::FromString(Item->ItemDisplayName.ToString().Left(2)), 20, WotUI::Title, true);
				if (UOverlaySlot* OS = Stack->AddChildToOverlay(Initial)) {
					OS->SetHorizontalAlignment(HAlign_Center);
					OS->SetVerticalAlignment(VAlign_Center);
				}
			}
			S->CountText = UWotMenuWidget::MakeText(WidgetTree, FText::AsNumber(Item->Count), 13, WotUI::Text, true);
			if (UOverlaySlot* OS = Stack->AddChildToOverlay(S->CountText)) {
				OS->SetHorizontalAlignment(HAlign_Right);
				OS->SetVerticalAlignment(VAlign_Bottom);
				OS->SetPadding(FMargin(0, 0, 6.0f, 3.0f));
			}
			Weight += Item->Weight * Item->Count;
			Count += Item->Count;
		}
		S->Button->AddChild(Cell);
		S->Button->OnClicked.AddDynamic(S, &UWotInventorySlot::OnClicked);
		Grid->AddChildToUniformGrid(S->Button, i / Columns, i % Columns);
		Slots.Add(S);
	}
	if (WeightBlock) {
		WeightBlock->SetText(FText::Format(NSLOCTEXT("Wot", "InvSummary", "{0} items   {1} kg"), Count, FText::AsNumber(FMath::RoundToInt(Weight))));
	}
	if (Selected >= Num) {
		Selected = Num > 0 ? Num - 1 : -1;
	}
	UWotItem* Item = SelectedItem();
	if (DetailName) {
		DetailName->SetText(Item ? Item->ItemDisplayName : NSLOCTEXT("Wot", "SelectItem", "Select an item"));
	}
	if (DetailCount) {
		DetailCount->SetText(Item ? FText::Format(NSLOCTEXT("Wot", "ItemCountWeight", "x{0}   {1} kg each"), Item->Count, FText::AsNumber(Item->Weight)) : FText::GetEmpty());
	}
	if (DetailDescription) {
		DetailDescription->SetText(Item ? Item->ItemDescription : NSLOCTEXT("Wot", "InvHint", "Click an item to see what it does. Arrow keys move the selection, Enter uses it."));
	}
	if (DetailIcon) {
		if (Item && Item->Thumbnail) {
			DetailIcon->SetBrushFromTexture(Item->Thumbnail, false);
			DetailIcon->SetColorAndOpacity(FLinearColor::White);
		} else {
			DetailIcon->SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
		}
	}
	if (UseButtonText) {
		UseButtonText->SetText(Item && !Item->UseActionText.IsEmpty() ? Item->UseActionText : NSLOCTEXT("Wot", "Use", "Use"));
	}
	const bool bOwn = IsOwningPlayersInventory();
	if (UseButton) {
		UseButton->SetIsEnabled(Item != nullptr);
	}
	if (DropButton) {
		DropButton->SetIsEnabled(Item != nullptr && bOwn);
		DropButton->SetVisibility(bOwn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UWotInventoryScreen::SelectIndex(int32 Index)
{
	Selected = Index;
	Refresh();
}

void UWotInventoryScreen::OnUseClicked()
{
	UWotItem* Item = SelectedItem();
	ACharacter* Character = GetOwningPlayerPawn() ? Cast<ACharacter>(GetOwningPlayerPawn()) : nullptr;
	if (Item && Character) {
		if (IsOwningPlayersInventory()) {
			Item->Use(Character);
		} else {
			Item->UseAddedToInventory(Character);
		}
	}
	Refresh();
}

void UWotInventoryScreen::OnDropClicked()
{
	UWotItem* Item = SelectedItem();
	APawn* Pawn = GetOwningPlayerPawn();
	if (Item && Pawn) {
		Item->Drop(Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 120.0f, 1);
	}
	Refresh();
}

void UWotInventoryScreen::OnCloseClicked()
{
	Close();
}

void UWotInventoryScreen::Close_Implementation()
{
	if (APlayerController* PC = GetOwningPlayer()) {
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);   // menus that stay open show it again themselves
	}
	// the character gates movement, camera and channeling on IsInventoryWidgetOpen(): clear its reference whichever way
	// the screen was closed (Esc / I / the button, not only the Blueprint toggle)
	if (AWotCharacter* Char = Cast<AWotCharacter>(GetOwningPlayerPawn())) {
		if (Char->IsInventoryWidgetOpen()) {
			Char->CloseInventoryWidget();
		}
	}
	if (InventoryComp) {
		InventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UWotUWInventoryPanel::UpdateInventory);
	}
	bBound = false;
	RemoveFromParent();
}
