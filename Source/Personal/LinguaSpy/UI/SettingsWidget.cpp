// LinguaSpy - settings overlay: volumes, translation and readings

#include "SettingsWidget.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpySettings"

FText USettingsWidget::GetTitle() const
{
	return LOCTEXT("Title", "SETTINGS");
}

ULinguaSpySaveSubsystem* USettingsWidget::GetSave() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
}

USlider* USettingsWidget::AddSliderRow(const FText& Label, float Value, UTextBlock*& OutValueText)
{
	using namespace LinguaSpyStyle;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelSize->SetWidthOverride(150.f);
	LabelSize->SetContent(MakeText(Label, 19, TextMain, true));
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelSize))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(0.f);
	Slider->SetMaxValue(1.f);
	Slider->SetStepSize(0.05f);
	Slider->SetValue(Value);
	Slider->SetSliderBarColor(TextMuted);
	Slider->SetSliderHandleColor(Accent);
	Slider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleSliderChanged);
	if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider))
	{
		SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SliderSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ValueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValueSize->SetWidthOverride(70.f);
	OutValueText = MakeText(FText::GetEmpty(), 18, TextMuted);
	OutValueText->SetJustification(ETextJustify::Right);
	ValueSize->SetContent(OutValueText);
	if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueSize))
	{
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}
	ValueTexts.Add(OutValueText);

	AddRow(ContentBox, Row, 14.f);
	return Slider;
}

void USettingsWidget::BuildContent()
{
	using namespace LinguaSpyStyle;
	const ULinguaSpySaveSubsystem* Save = GetSave();
	const FLinguaSpySettings Settings = Save ? Save->GetSettings() : FLinguaSpySettings();

	AddRow(ContentBox, MakeText(LOCTEXT("Audio", "Audio"), 22, Accent, true), 0.f);
	UTextBlock* ValueText = nullptr;
	MasterSlider = AddSliderRow(LOCTEXT("Master", "Master"), Settings.MasterVolume, ValueText);
	MusicSlider = AddSliderRow(LOCTEXT("Music", "Music"), Settings.MusicVolume, ValueText);
	AmbienceSlider = AddSliderRow(LOCTEXT("Ambience", "Ambience"), Settings.AmbienceVolume, ValueText);
	UISlider = AddSliderRow(LOCTEXT("UI", "Interface"), Settings.UIVolume, ValueText);

	AddRow(ContentBox, MakeText(LOCTEXT("Language", "Language help"), 22, Accent, true), 28.f);

	UTextBlock* Label = nullptr;
	UButton* TranslationButton = MakeButton(FText::GetEmpty(), &Label);
	TranslationLabel = Label;
	TranslationButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleTranslationToggle);
	AddRow(ContentBox, TranslationButton, 12.f);
	AddRow(ContentBox, MakeText(LOCTEXT("TranslationHint",
		"Off: tap Translate on a line when you need the helper. Answers given after peeking count as assisted and lower your mission grade."),
		15, TextMuted), 6.f);

	UButton* ReadingsButton = MakeButton(FText::GetEmpty(), &Label);
	ReadingsLabel = Label;
	ReadingsButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleReadingsToggle);
	AddRow(ContentBox, ReadingsButton, 14.f);

	RefreshLabels();
}

void USettingsWidget::RefreshLabels()
{
	const ULinguaSpySaveSubsystem* Save = GetSave();
	const FLinguaSpySettings Settings = Save ? Save->GetSettings() : FLinguaSpySettings();

	const float Values[] = { Settings.MasterVolume, Settings.MusicVolume, Settings.AmbienceVolume, Settings.UIVolume };
	for (int32 i = 0; i < ValueTexts.Num() && i < UE_ARRAY_COUNT(Values); ++i)
	{
		ValueTexts[i]->SetText(FText::AsPercent(Values[i]));
	}
	if (TranslationLabel)
	{
		TranslationLabel->SetText(Settings.bAlwaysShowTranslation
			? LOCTEXT("TranslationOn", "Always show translations: ON")
			: LOCTEXT("TranslationOff", "Always show translations: OFF"));
	}
	if (ReadingsLabel)
	{
		ReadingsLabel->SetText(Settings.bShowReadings
			? LOCTEXT("ReadingsOn", "Show readings (romaji): ON")
			: LOCTEXT("ReadingsOff", "Show readings (romaji): OFF"));
	}
}

void USettingsWidget::HandleSliderChanged(float Value)
{
	if (ULinguaSpySaveSubsystem* Save = GetSave())
	{
		Save->SetVolumes(MasterSlider->GetValue(), MusicSlider->GetValue(), AmbienceSlider->GetValue(), UISlider->GetValue());
	}
	RefreshLabels();
}

void USettingsWidget::HandleTranslationToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetSave())
	{
		Save->SetAlwaysShowTranslation(!Save->GetSettings().bAlwaysShowTranslation);
	}
	RefreshLabels();
}

void USettingsWidget::HandleReadingsToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetSave())
	{
		Save->SetShowReadings(!Save->GetSettings().bShowReadings);
	}
	RefreshLabels();
}

void USettingsWidget::Close()
{
	if (ULinguaSpySaveSubsystem* Save = GetSave())
	{
		Save->SaveSettings();
	}
	Super::Close();
}

#undef LOCTEXT_NAMESPACE
