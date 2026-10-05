// LinguaSpy - settings overlay: volumes, translation and readings

#include "SettingsWidget.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "LanguagePackAsset.h"
#include "LearningTelemetrySubsystem.h"
#include "MissionSubsystem.h"
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
	VoiceSlider = AddSliderRow(LOCTEXT("Voice", "Voices"), Settings.VoiceVolume, ValueText);

	AddRow(ContentBox, MakeText(LOCTEXT("Language", "Language help"), 22, Accent, true), 28.f);

	UTextBlock* Label = nullptr;
	UButton* HandlerButton = MakeButton(FText::GetEmpty(), &Label);
	TranslationLabel = Label;
	HandlerButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleHandlerToggle);
	AddRow(ContentBox, HandlerButton, 12.f);
	HandlerHint = MakeText(FText::GetEmpty(), 15, TextMuted);
	AddRow(ContentBox, HandlerHint, 6.f);

	UButton* ReadingsButton = MakeButton(FText::GetEmpty(), &Label);
	ReadingsLabel = Label;
	ReadingsButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleReadingsToggle);
	AddRow(ContentBox, ReadingsButton, 14.f);

	AddRow(ContentBox, MakeText(LOCTEXT("Privacy", "Learning log"), 22, Accent, true), 28.f);
	UButton* TelemetryButton = MakeButton(FText::GetEmpty(), &Label);
	TelemetryLabel = Label;
	TelemetryButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleTelemetryToggle);
	AddRow(ContentBox, TelemetryButton, 12.f);
	AddRow(ContentBox, MakeText(LOCTEXT("TelemetryHint",
		"Saves your answers, hints and timings to a file on this computer (Saved/Telemetry) so you or a teacher can see "
		"what's working. Nothing is ever sent anywhere."), 15, TextMuted), 6.f);
	UButton* LogsButton = MakeButton(LOCTEXT("OpenLogs", "Open log folder"));
	LogsButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleOpenLogs);
	AddRow(ContentBox, LogsButton, 8.f);

	RefreshLabels();
}

void USettingsWidget::RefreshLabels()
{
	const ULinguaSpySaveSubsystem* Save = GetSave();
	const FLinguaSpySettings Settings = Save ? Save->GetSettings() : FLinguaSpySettings();

	const float Values[] = { Settings.MasterVolume, Settings.MusicVolume, Settings.AmbienceVolume, Settings.UIVolume, Settings.VoiceVolume };
	for (int32 i = 0; i < ValueTexts.Num() && i < UE_ARRAY_COUNT(Values); ++i)
	{
		ValueTexts[i]->SetText(FText::AsPercent(Values[i]));
	}
	if (TranslationLabel && Save)
	{
		const FName City = GetHandlerCity();
		const TArray<FName> Cities = GetAllCityIds();
		const EHandlerMode Mode = Save->GetHandlerMode(City.IsNone() && !Cities.IsEmpty() ? Cities[0] : City);
		const FText ModeName = Mode == EHandlerMode::Shadowing ? LOCTEXT("Shadowing", "Shadowing")
			: Mode == EHandlerMode::OnCall ? LOCTEXT("OnCall", "On call") : LOCTEXT("RadioSilenceMode", "Radio silence");
		TranslationLabel->SetText(City.IsNone()
			? FText::Format(LOCTEXT("HandlerAll", "Handler (all cities): {0}"), ModeName)
			: FText::Format(LOCTEXT("HandlerCity", "Handler in {0}: {1}"), FText::FromName(City), ModeName));
		if (HandlerHint)
		{
			HandlerHint->SetText(Mode == EHandlerMode::Shadowing
				? LOCTEXT("ShadowingHint", "Shadowing: every line is translated for you. Best for your first missions in a language.")
				: Mode == EHandlerMode::OnCall
				? LOCTEXT("OnCallHint", "On call: tap Translate when you need it (lowers your grade a little).")
				: LOCTEXT("RadioHint", "Radio silence: only 2 translations per conversation. For confident agents."));
		}
	}
	if (TelemetryLabel)
	{
		TelemetryLabel->SetText(Settings.bTelemetryEnabled
			? LOCTEXT("TelemetryOn", "Keep a learning log: ON")
			: LOCTEXT("TelemetryOff", "Keep a learning log: OFF"));
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
		Save->SetVoiceVolume(VoiceSlider->GetValue());
	}
	RefreshLabels();
}

FName USettingsWidget::GetHandlerCity() const
{
	const UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr;
	return Missions ? Missions->GetCurrentCityId() : NAME_None;
}

TArray<FName> USettingsWidget::GetAllCityIds() const
{
	TArray<FName> Out;
	if (const UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr)
	{
		for (const ULanguagePackAsset* Pack : Missions->GetAllLanguagePacks())
		{
			Out.Add(Pack->CityId);
		}
	}
	return Out;
}

void USettingsWidget::HandleHandlerToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	ULinguaSpySaveSubsystem* Save = GetSave();
	if (!Save)
	{
		return;
	}
	const FName City = GetHandlerCity();
	const TArray<FName> Cities = City.IsNone() ? GetAllCityIds() : TArray<FName>{ City };
	if (Cities.IsEmpty())
	{
		return;
	}
	const EHandlerMode Next = static_cast<EHandlerMode>((static_cast<int32>(Save->GetHandlerMode(Cities[0])) + 1) % 3);
	for (const FName Id : Cities)
	{
		Save->SetHandlerMode(Id, Next);
	}
	RefreshLabels();
}

void USettingsWidget::HandleTelemetryToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetSave())
	{
		Save->SetTelemetryEnabled(!Save->GetSettings().bTelemetryEnabled);
	}
	RefreshLabels();
}

void USettingsWidget::HandleOpenLogs()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (const ULearningTelemetrySubsystem* Telemetry = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULearningTelemetrySubsystem>() : nullptr)
	{
		Telemetry->OpenLogFolder();
	}
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
