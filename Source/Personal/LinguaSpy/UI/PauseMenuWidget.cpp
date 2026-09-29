// LinguaSpy - in-city pause menu

#include "PauseMenuWidget.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "MissionSubsystem.h"
#include "PhrasebookWidget.h"
#include "SettingsWidget.h"
#include "FluencyScoring.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "LinguaSpyPause"

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (MenuMap.IsNull())
	{
		MenuMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/LinguaSpy/Maps/Lvl_MainMenu.Lvl_MainMenu")));
	}
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (ResumeButton)   { ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResume); }
	if (PhrasebookButton) { PhrasebookButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandlePhrasebook); }
	if (SettingsButton)   { SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSettings); }
	if (MenuButton)     { MenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMenu); }
	RefreshLabels();
}

void UPauseMenuWidget::NativeDestruct()
{
	for (const TWeakObjectPtr<UOverlayPanelWidget>& Overlay : OpenOverlays)
	{
		if (Overlay.IsValid() && Overlay->IsInViewport())
		{
			Overlay->Close();
		}
	}
	OpenOverlays.Reset();
	Super::NativeDestruct();
}

void UPauseMenuWidget::BuildDefaultLayout()
{
	using namespace LinguaSpyStyle;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	LinguaSpyStyle::StyleBorder(Dim, FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f);
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	LinguaSpyStyle::StyleBorder(Panel, PanelBg, LinguaSpyStyle::PanelRadius);
	Panel->SetPadding(FMargin(40.f, 30.f));
	Dim->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	StyleText(Title, 32, Accent, true);
	Title->SetText(LOCTEXT("Paused", "PAUSED"));
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	TallyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TallyText"));
	StyleText(TallyText, 15, TextMuted);
	TallyText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TallySlot = Column->AddChildToVerticalBox(TallyText))
	{
		TallySlot->SetHorizontalAlignment(HAlign_Center);
		TallySlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	auto MakeButton = [this, Column](const TCHAR* Name, UTextBlock*& OutLabel) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		LinguaSpyStyle::StyleButton(Button, LinguaSpyStyle::ButtonRadius);
		Button->SetBackgroundColor(LinguaSpyStyle::Accent);
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LinguaSpyStyle::StyleText(OutLabel, 18, LinguaSpyStyle::TileText, true);
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(OutLabel)))
		{
			ButtonSlot->SetPadding(FMargin(24.f, 10.f));
		}
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Button))
		{
			RowSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	};

	UTextBlock* ResumeLabel = nullptr;
	UTextBlock* MenuLabel = nullptr;
	ResumeButton = MakeButton(TEXT("ResumeButton"), ResumeLabel);
	ResumeLabel->SetText(LOCTEXT("Resume", "Resume"));
	UTextBlock* PhrasebookLabel = nullptr;
	PhrasebookButton = MakeButton(TEXT("PhrasebookButton"), PhrasebookLabel);
	PhrasebookLabel->SetText(LOCTEXT("Phrasebook", "Phrasebook"));
	UTextBlock* SettingsLabel = nullptr;
	SettingsButton = MakeButton(TEXT("SettingsButton"), SettingsLabel);
	SettingsLabel->SetText(LOCTEXT("Settings", "Settings"));
	MenuButton = MakeButton(TEXT("MenuButton"), MenuLabel);
	MenuLabel->SetText(LOCTEXT("Menu", "Return to menu"));
}

void UPauseMenuWidget::RefreshLabels()
{
	const UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!TallyText)
	{
		return;
	}
	if (!Missions || !Missions->GetActiveMission())
	{
		TallyText->SetText(FText::GetEmpty());
		return;
	}
	const FMissionGrade Tally = Missions->GetCurrentMissionTally();
	TallyText->SetText(FText::Format(LOCTEXT("Tally", "{0}: {1} {1}|plural(one=answer,other=answers), {2} {2}|plural(one=mistake,other=mistakes), {3} {3}|plural(one=peek,other=peeks) - on track for {4}"),
		Missions->GetActiveMission()->Title, Tally.Challenges, Tally.Mistakes, Tally.Peeks,
		UFluencyScoring::GradeLetter(UFluencyScoring::ComputeGrade(Tally.Challenges, Tally.Mistakes, Tally.Peeks))));
}

void UPauseMenuWidget::HandleResume()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	OnResume.Broadcast();
}

void UPauseMenuWidget::HandlePhrasebook()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>();
	if (UPhrasebookWidget* Phrasebook = CreateWidget<UPhrasebookWidget>(GetOwningPlayer(), UPhrasebookWidget::StaticClass()))
	{
		Phrasebook->SetPack(Missions ? Missions->GetCurrentPack() : nullptr);
		Phrasebook->Open();
		OpenOverlays.Add(Phrasebook);
	}
}

void UPauseMenuWidget::HandleSettings()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (USettingsWidget* Settings = CreateWidget<USettingsWidget>(GetOwningPlayer(), USettingsWidget::StaticClass()))
	{
		Settings->Open();
		OpenOverlays.Add(Settings);
	}
}

void UPauseMenuWidget::HandleMenu()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->LeaveCity();
	}
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, MenuMap);
}

#undef LOCTEXT_NAMESPACE
