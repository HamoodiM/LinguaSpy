// LinguaSpy - title screen with city select

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UWrapBox;
class UTextBlock;
class UCityCardWidget;
class ULanguagePackAsset;

/** Title + one card per language pack + reset/quit. Builds a default layout in C++. */
UCLASS()
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu")
	TSubclassOf<UCityCardWidget> CardClass;

	UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> CardBox;

	UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

private:

	void BuildDefaultLayout();
	void RefreshCards();

	UFUNCTION()
	void HandleCityChosen(ULanguagePackAsset* Pack);

	UFUNCTION()
	void HandleReset();

	UFUNCTION()
	void HandleQuit();

	bool bResetArmed = false;
};
