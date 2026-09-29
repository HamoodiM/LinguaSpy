// LinguaSpy - title screen with city select

#include "MainMenuWidget.h"
#include "CityCardWidget.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "MissionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "LinguaSpyMenu"

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!CardClass)
	{
		CardClass = UCityCardWidget::StaticClass();
	}
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (ResetButton) { ResetButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleReset); }
	if (QuitButton) { QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuit); }
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshCards();
}

void UMainMenuWidget::BuildDefaultLayout()
{
	using namespace LinguaSpyStyle;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(PanelBg.R, PanelBg.G, PanelBg.B, 0.55f));
	Background->SetPadding(FMargin(60.f, 50.f));
	Background->SetHorizontalAlignment(HAlign_Center);
	Background->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Background;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Background->SetContent(Column);

	auto AddCentered = [Column](UWidget* Widget, float TopPadding)
	{
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}
	};

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	StyleText(Title, 64, Accent, true);
	Title->SetText(LOCTEXT("Title", "LINGUASPY"));
	AddCentered(Title, 0.f);

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
	StyleText(Subtitle, 20, TextMain);
	Subtitle->SetText(LOCTEXT("Subtitle", "Learn the language. Keep your cover. Choose your city."));
	AddCentered(Subtitle, 6.f);

	CardBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("CardBox"));
	CardBox->SetInnerSlotPadding(FVector2D(20.f, 20.f));
	// Lay cards out in rows instead of one per line
	CardBox->SetExplicitWrapSize(true);
	CardBox->SetWrapSize(1180.f);
	AddCentered(CardBox, 40.f);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StyleText(StatusText, 16, TextMuted);
	AddCentered(StatusText, 24.f);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto MakeButton = [this, Buttons](const FText& Label, const TCHAR* Name) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetBackgroundColor(LinguaSpyStyle::Accent);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LinguaSpyStyle::StyleText(Text, 18, LinguaSpyStyle::TileText, true);
		Text->SetText(Label);
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
		{
			ButtonSlot->SetPadding(FMargin(20.f, 8.f));
		}
		if (UHorizontalBoxSlot* ButtonRowSlot = Buttons->AddChildToHorizontalBox(Button))
		{
			ButtonRowSlot->SetPadding(FMargin(8.f, 0.f));
		}
		return Button;
	};
	ResetButton = MakeButton(LOCTEXT("Reset", "Reset progress"), TEXT("ResetButton"));
	QuitButton = MakeButton(LOCTEXT("Quit", "Quit"), TEXT("QuitButton"));
	AddCentered(Buttons, 16.f);
}

void UMainMenuWidget::RefreshCards()
{
	if (!CardBox)
	{
		return;
	}
	CardBox->ClearChildren();

	UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Missions)
	{
		return;
	}
	const TArray<ULanguagePackAsset*> Packs = Missions->GetAllLanguagePacks();
	for (ULanguagePackAsset* Pack : Packs)
	{
		int32 Completed = 0;
		int32 Total = 0;
		Missions->GetCityCompletion(Pack, Completed, Total);

		UCityCardWidget* Card = CreateWidget<UCityCardWidget>(this, CardClass);
		Card->SetPack(Pack, Completed, Total);
		Card->OnCardClicked.AddDynamic(this, &UMainMenuWidget::HandleCityChosen);
		CardBox->AddChildToWrapBox(Card);
	}
	if (StatusText)
	{
		StatusText->SetText(Packs.IsEmpty() ? LOCTEXT("NoPacks", "No cities found.") : FText::GetEmpty());
	}
}

void UMainMenuWidget::HandleCityChosen(ULanguagePackAsset* Pack)
{
	if (!Pack || Pack->CityMap.IsNull())
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->SelectLanguagePack(Pack);
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Pack->CityMap);
}

void UMainMenuWidget::HandleReset()
{
	// Two clicks to confirm
	if (!bResetArmed)
	{
		bResetArmed = true;
		if (StatusText)
		{
			StatusText->SetText(LOCTEXT("ResetConfirm", "Click 'Reset progress' again to erase all mission and skill progress."));
		}
		return;
	}
	bResetArmed = false;
	if (ULinguaSpySaveSubsystem* Save = GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>())
	{
		Save->ResetProgress();
	}
	RefreshCards();
	if (StatusText)
	{
		StatusText->SetText(LOCTEXT("ResetDone", "Progress reset."));
	}
}

void UMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
