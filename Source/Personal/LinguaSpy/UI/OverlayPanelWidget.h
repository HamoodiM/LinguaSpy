// LinguaSpy - base for full-screen overlay panels (settings, phrasebook, safe house)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverlayPanelWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOverlayClosed);

/**
 *  Dimmed full-screen backdrop + centred rounded panel with a title and a Close button.
 *  Subclasses fill ContentBox in BuildContent(). Close() removes the widget and broadcasts OnClosed.
 */
UCLASS(Abstract)
class UOverlayPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Overlay")
	FOnOverlayClosed OnClosed;

	UFUNCTION(BlueprintCallable, Category="Overlay")
	virtual void Close();

	/** Adds the overlay to the viewport above menus/pause (Z 120) and focuses it. */
	void Open();

protected:

	virtual void NativeOnInitialized() override;

	/** Called once from NativeOnInitialized, after the frame exists. */
	virtual void BuildContent() {}

	virtual FText GetTitle() const { return FText::GetEmpty(); }
	virtual float GetPanelWidth() const { return 760.f; }

	void SetTitle(const FText& Title);

	// ---- Small builders shared by the panels ----
	UTextBlock* MakeText(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
	UButton* MakeButton(const FText& Label, UTextBlock** OutLabel = nullptr, int32 FontSize = 18);
	UProgressBar* MakeBar(float Percent, const FLinearColor& Fill, float Height = 12.f);
	void AddRow(UVerticalBox* Box, UWidget* Widget, float TopPadding);

	UPROPERTY(BlueprintReadOnly, Category="Overlay", meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(BlueprintReadOnly, Category="Overlay", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category="Overlay", meta=(BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** The dimmed backdrop (subclasses may hide it while showing something else). */
	UPROPERTY()
	TObjectPtr<UBorder> Backdrop;

private:

	UFUNCTION()
	void HandleCloseClicked();
};
