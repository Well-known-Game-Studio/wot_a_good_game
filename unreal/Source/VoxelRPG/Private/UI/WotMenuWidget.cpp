#include "UI/WotMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

FSlateBrush WotUI::RoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Fill);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	return Brush;
}

void UWotMenuWidget::StyleButton(UButton* Button, const FLinearColor& Normal)
{
	if (!Button) {
		return;
	}
	FButtonStyle Style;
	Style.Normal = WotUI::RoundedBrush(Normal, 6.0f, WotUI::PanelLine * FLinearColor(1, 1, 1, 0.6f), 1.0f);
	Style.Hovered = WotUI::RoundedBrush(WotUI::ButtonHover, 6.0f, WotUI::Title, 1.5f);
	Style.Pressed = WotUI::RoundedBrush(WotUI::ButtonPressed, 6.0f, WotUI::Title, 1.5f);
	Style.Disabled = WotUI::RoundedBrush(Normal * FLinearColor(1, 1, 1, 0.4f), 6.0f, WotUI::Muted, 1.0f);
	Style.NormalPadding = FMargin(14.0f, 8.0f);
	Style.PressedPadding = FMargin(14.0f, 9.0f, 14.0f, 7.0f);
	Button->SetStyle(Style);
}

UTextBlock* UWotMenuWidget::MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetText(Text);
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = Size;
	if (bBold) {
		Font.TypefaceFontName = FName("Bold");
	}
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Block->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.7f));
	return Block;
}

TSharedRef<SWidget> UWotMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		// backdrop
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MenuRoot"));
		WidgetTree->RootWidget = Root;
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrushColor(WotUI::Backdrop);
		if (UOverlaySlot* S = Root->AddChildToOverlay(Backdrop)) {
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}
		// panel
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
		Size->SetWidthOverride(PanelWidth);
		if (UOverlaySlot* S = Root->AddChildToOverlay(Size)) {
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetVerticalAlignment(VAlign_Center);
		}
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WotUI::RoundedBrush(WotUI::Panel, 10.0f, WotUI::PanelLine, 2.0f));
		Panel->SetPadding(FMargin(28.0f, 22.0f));
		Size->AddChild(Panel);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		Panel->SetContent(Column);
		TitleBlock = MakeText(WidgetTree, TitleText, 30, WotUI::Title, true);
		TitleBlock->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(TitleBlock)) {
			S->SetPadding(FMargin(0, 0, 0, 16.0f));
			S->SetHorizontalAlignment(HAlign_Fill);
		}
		Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		Column->AddChildToVerticalBox(Content);
		BuildContent();
	}
	return Super::RebuildWidget();
}

UButton* UWotMenuWidget::AddButton(const FText& Label, const FLinearColor& Color, float FontSize)
{
	if (!Content || !WidgetTree) {
		return nullptr;
	}
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	StyleButton(Button, Color);
	UTextBlock* Text = MakeText(WidgetTree, Label, FontSize, WotUI::Text);
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	if (UButtonSlot* BS = Cast<UButtonSlot>(Text->Slot)) {
		BS->SetHorizontalAlignment(HAlign_Fill);
	}
	if (UVerticalBoxSlot* S = Content->AddChildToVerticalBox(Button)) {
		S->SetPadding(FMargin(0, 4.0f));
		S->SetHorizontalAlignment(HAlign_Fill);
	}
	if (!FirstButton) {
		FirstButton = Button;
	}
	return Button;
}

UTextBlock* UWotMenuWidget::AddLabel(const FText& Text, int32 Size, const FLinearColor& Color)
{
	if (!Content || !WidgetTree) {
		return nullptr;
	}
	UTextBlock* Block = MakeText(WidgetTree, Text, Size, Color);
	Block->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Content->AddChildToVerticalBox(Block)) {
		S->SetPadding(FMargin(2.0f, 4.0f));
	}
	return Block;
}

UHorizontalBox* UWotMenuWidget::AddRow()
{
	if (!Content || !WidgetTree) {
		return nullptr;
	}
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* S = Content->AddChildToVerticalBox(Row)) {
		S->SetPadding(FMargin(0, 3.0f));
		S->SetHorizontalAlignment(HAlign_Fill);
	}
	return Row;
}

void UWotMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void UWotMenuWidget::NativeDestruct()
{
	if (bOpen) {
		Close();
	}
	Super::NativeDestruct();
}

FReply UWotMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right) {
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWotMenuWidget::Open()
{
	if (bOpen) {
		return;
	}
	bOpen = true;
	AddToViewport(100);
	APlayerController* PC = GetOwningPlayer();
	if (PC) {
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	if (bPausesGame) {
		bWasPaused = UGameplayStatics::IsGamePaused(this);
		UGameplayStatics::SetGamePaused(this, true);
	}
	if (FirstButton) {
		FirstButton->SetKeyboardFocus();
	} else {
		SetKeyboardFocus();
	}
}

void UWotMenuWidget::Close()
{
	if (!bOpen) {
		return;
	}
	bOpen = false;
	if (bPausesGame) {
		UGameplayStatics::SetGamePaused(this, bWasPaused);
	}
	APlayerController* PC = GetOwningPlayer();
	if (PC) {
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	RemoveFromParent();
}

UWotMenuWidget* UWotMenuWidget::OpenMenu(UObject* WorldContextObject, TSubclassOf<UWotMenuWidget> MenuClass)
{
	if (!WorldContextObject || !MenuClass) {
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC) {
		return nullptr;
	}
	UWotMenuWidget* Menu = CreateWidget<UWotMenuWidget>(PC, MenuClass);
	if (Menu) {
		Menu->Open();
	}
	return Menu;
}

void UWotMenuWidget::CloseAllMenus(UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) {
		return;
	}
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UWotMenuWidget::StaticClass(), false);
	for (UUserWidget* W : Found) {
		if (UWotMenuWidget* M = Cast<UWotMenuWidget>(W)) {
			M->Close();
		}
	}
}
