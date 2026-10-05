// LinguaSpy - settings overlay: volumes, translation and readings

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "SettingsWidget.generated.h"

class USlider;
class ULinguaSpySaveSubsystem;

/**
 *  Master / Music / Ambience / Voice / UI sliders (applied live), handler support mode (current city, or every city from
 *  the main menu), readings toggle, and the local learning log toggle. Saved on close.
 */
UCLASS()
class USettingsWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	virtual void Close() override;

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual float GetPanelWidth() const override { return 620.f; }

private:

	USlider* AddSliderRow(const FText& Label, float Value, UTextBlock*& OutValueText);
	void RefreshLabels();
	ULinguaSpySaveSubsystem* GetSave() const;

	UFUNCTION()
	void HandleSliderChanged(float Value);

	UFUNCTION()
	void HandleHandlerToggle();

	UFUNCTION()
	void HandleTelemetryToggle();

	UFUNCTION()
	void HandleOpenLogs();

	/** City whose handler mode is shown (None = from the menu: changes apply to every city). */
	FName GetHandlerCity() const;
	TArray<FName> GetAllCityIds() const;

	UFUNCTION()
	void HandleReadingsToggle();

	UPROPERTY()
	TObjectPtr<USlider> MasterSlider;

	UPROPERTY()
	TObjectPtr<USlider> MusicSlider;

	UPROPERTY()
	TObjectPtr<USlider> AmbienceSlider;

	UPROPERTY()
	TObjectPtr<USlider> UISlider;

	UPROPERTY()
	TObjectPtr<USlider> VoiceSlider;

	UPROPERTY()
	TObjectPtr<UTextBlock> TelemetryLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> HandlerHint;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ValueTexts;

	UPROPERTY()
	TObjectPtr<UTextBlock> TranslationLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> ReadingsLabel;
};
