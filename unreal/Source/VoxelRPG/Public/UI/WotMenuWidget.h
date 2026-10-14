#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WotMenuWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
class USizeBox;

/** Shared look for the C++ built menus: dark parchment panels, gold titles, rounded buttons. */
namespace WotUI
{
	const FLinearColor Backdrop(0.0f, 0.0f, 0.0f, 0.55f);
	const FLinearColor Panel(0.09f, 0.075f, 0.06f, 0.96f);
	const FLinearColor PanelLine(0.55f, 0.42f, 0.18f, 1.0f);
	const FLinearColor Title(1.0f, 0.85f, 0.45f, 1.0f);
	const FLinearColor Text(0.92f, 0.88f, 0.80f, 1.0f);
	const FLinearColor Muted(0.62f, 0.58f, 0.50f, 1.0f);
	const FLinearColor ButtonNormal(0.17f, 0.14f, 0.11f, 1.0f);
	const FLinearColor ButtonHover(0.30f, 0.24f, 0.14f, 1.0f);
	const FLinearColor ButtonPressed(0.45f, 0.35f, 0.16f, 1.0f);
	const FLinearColor Selected(0.36f, 0.28f, 0.12f, 1.0f);
	const FLinearColor Danger(0.45f, 0.14f, 0.12f, 1.0f);
	/** Rounded, outlined panel brush shared by every C++ widget. */
	VOXELRPG_API FSlateBrush RoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth);
}

/**
 * Base for menus built entirely in C++ (no UMG asset needed). Subclasses
 * override BuildContent() and add rows to Content. The widget pauses the
 * game and switches to UI input while open when bPausesGame is set.
 */
UCLASS(Abstract)
class VOXELRPG_API UWotMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FText TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	float PanelWidth = 560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	bool bPausesGame = true;

	/** Opens the menu for the given player (adds to viewport, input mode, pause). */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	virtual void Close();

	/** Helper for subclasses and Blueprints: create, open and return a menu of the given class. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "MenuClass"))
	static UWotMenuWidget* OpenMenu(UObject* WorldContextObject, TSubclassOf<UWotMenuWidget> MenuClass);

	/** Closes every open C++ menu for the local player. */
	UFUNCTION(BlueprintCallable, Category = "Menu", meta = (WorldContext = "WorldContextObject"))
	static void CloseAllMenus(UObject* WorldContextObject);

	/** Applies the shared button style. */
	static void StyleButton(UButton* Button, const FLinearColor& Normal = WotUI::ButtonNormal);
	static UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = false);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Subclasses fill Content here (called from RebuildWidget after the frame exists). */
	virtual void BuildContent() {}

	/** Adds a full-width menu button with a label; returns it so the caller can bind OnClicked. */
	UButton* AddButton(const FText& Label, const FLinearColor& Color = WotUI::ButtonNormal, float FontSize = 20.0f);
	UTextBlock* AddLabel(const FText& Text, int32 Size = 16, const FLinearColor& Color = WotUI::Text);
	UHorizontalBox* AddRow();

	UPROPERTY()
	TObjectPtr<UVerticalBox> Content;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleBlock;

	UPROPERTY()
	TObjectPtr<UButton> FirstButton;

	bool bWasPaused = false;
	bool bOpen = false;
};
