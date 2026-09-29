// LinguaSpy - in-city pause menu

#include "PauseMenuWidget.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "MissionSubsystem.h"
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
	if (HelperButton)   { HelperButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleHelper); }
	if (ReadingsButton) { ReadingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleReadings); }
	if (MenuButton)     { MenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMenu); }
	RefreshLabels();
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
	UTextBlock* Helper = nullptr;
	HelperButton = MakeButton(TEXT("HelperButton"), Helper);
	HelperLabel = Helper;
	UTextBlock* Readings = nullptr;
	ReadingsButton = MakeButton(TEXT("ReadingsButton"), Readings);
	ReadingsLabel = Readings;
	MenuButton = MakeButton(TEXT("MenuButton"), MenuLabel);
	MenuLabel->SetText(LOCTEXT("Menu", "Return to menu"));
}

void UPauseMenuWidget::RefreshLabels()
{
	const ULinguaSpySaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
	if (!Save)
	{
		return;
	}
	if (HelperLabel)
	{
		HelperLabel->SetText(Save->GetSettings().bHelperEnabled ? LOCTEXT("HelperOn", "Helper translation: ON") : LOCTEXT("HelperOff", "Helper translation: OFF"));
	}
	if (ReadingsLabel)
	{
		ReadingsLabel->SetText(Save->GetSettings().bShowReadings ? LOCTEXT("ReadingsOn", "Show romaji: ON") : LOCTEXT("ReadingsOff", "Show romaji: OFF"));
	}
}

void UPauseMenuWidget::HandleResume()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	OnResume.Broadcast();
}

void UPauseMenuWidget::HandleHelper()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>())
	{
		Save->SetHelperEnabled(!Save->GetSettings().bHelperEnabled);
	}
	RefreshLabels();
}

void UPauseMenuWidget::HandleReadings()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>())
	{
		Save->SetShowReadings(!Save->GetSettings().bShowReadings);
	}
	RefreshLabels();
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
