// LinguaSpy - settings overlay: volumes, translation and readings

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "SettingsWidget.generated.h"

class USlider;
class ULinguaSpySaveSubsystem;

/** Master / Music / Ambience / UI sliders (applied live) and the translation / readings toggles. Saved on close. */
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
	void HandleTranslationToggle();

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
	TArray<TObjectPtr<UTextBlock>> ValueTexts;

	UPROPERTY()
	TObjectPtr<UTextBlock> TranslationLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> ReadingsLabel;
};
