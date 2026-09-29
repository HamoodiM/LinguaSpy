// LinguaSpy - one city on the city-select screen

#include "CityCardWidget.h"
#include "FluencyScoring.h"
#include "LanguagePackAsset.h"
#include "LanguageProgressSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "MissionAsset.h"
#include "MissionSubsystem.h"
#include "SpacedRepetitionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpyCityCard"

void UCityCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree->RootWidget)
	{
		using namespace LinguaSpyStyle;

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardSize"));
		CardSize->SetWidthOverride(260.f);
		CardSize->SetHeightOverride(320.f);
		Root->AddChildToVerticalBox(CardSize);

		CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CardButton"));
		LinguaSpyStyle::StyleButton(CardButton, LinguaSpyStyle::CardRadius);
		CardSize->SetContent(CardButton);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(CardButton->AddChild(Column)))
		{
			ButtonSlot->SetPadding(FMargin(20.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}

		auto AddText = [this, Column](const TCHAR* Name, int32 FontSize, bool bBold, float TopPadding) -> UTextBlock*
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
			StyleText(Text, FontSize, LinguaSpyStyle::TileText, bBold);
			Text->SetAutoWrapText(true);
			if (UVerticalBoxSlot* TextSlot = Column->AddChildToVerticalBox(Text))
			{
				TextSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
			}
			return Text;
		};
		CityText = AddText(TEXT("CityText"), 32, true, 0.f);
		LanguageText = AddText(TEXT("LanguageText"), 18, true, 2.f);
		TaglineText = AddText(TEXT("TaglineText"), 15, false, 14.f);
		ProgressText = AddText(TEXT("ProgressText"), 15, true, 14.f);
		DueText = AddText(TEXT("DueText"), 15, true, 8.f);
		WeakSkillText = AddText(TEXT("WeakSkillText"), 14, false, 4.f);

		// Safe house + phrasebook under the card
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardButtons"));
		auto MakeSmallButton = [this, Buttons](const FText& Label, const TCHAR* Name, float LeftPadding) -> UButton*
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			LinguaSpyStyle::StyleButton(Button, LinguaSpyStyle::ButtonRadius);
			Button->SetBackgroundColor(LinguaSpyStyle::TileNeutral);
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			LinguaSpyStyle::StyleText(Text, 15, LinguaSpyStyle::TileText, true);
			Text->SetText(Label);
			Text->SetJustification(ETextJustify::Center);
			if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
			{
				ButtonSlot->SetPadding(FMargin(8.f, 7.f));
			}
			if (UHorizontalBoxSlot* RowSlot = Buttons->AddChildToHorizontalBox(Button))
			{
				RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				RowSlot->SetPadding(FMargin(LeftPadding, 0.f, 0.f, 0.f));
			}
			return Button;
		};
		PracticeButton = MakeSmallButton(LOCTEXT("SafeHouse", "Safe house"), TEXT("PracticeButton"), 0.f);
		PhrasebookButton = MakeSmallButton(LOCTEXT("Phrasebook", "Phrasebook"), TEXT("PhrasebookButton"), 8.f);
		if (UVerticalBoxSlot* ButtonsSlot = Root->AddChildToVerticalBox(Buttons))
		{
			ButtonsSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		}
	}

	if (CardButton)       { CardButton->OnClicked.AddDynamic(this, &UCityCardWidget::HandleClicked); }
	if (PracticeButton)   { PracticeButton->OnClicked.AddDynamic(this, &UCityCardWidget::HandlePractice); }
	if (PhrasebookButton) { PhrasebookButton->OnClicked.AddDynamic(this, &UCityCardWidget::HandlePhrasebook); }
}

void UCityCardWidget::SetPack(ULanguagePackAsset* InPack, int32 CompletedMissions, int32 TotalMissions)
{
	Pack = InPack;
	if (!Pack)
	{
		return;
	}
	const bool bPlayable = !Pack->CityMap.IsNull();
	const UGameInstance* GameInstance = GetGameInstance();
	const UMissionSubsystem* Missions = GameInstance ? GameInstance->GetSubsystem<UMissionSubsystem>() : nullptr;
	const USpacedRepetitionSubsystem* Srs = GameInstance ? GameInstance->GetSubsystem<USpacedRepetitionSubsystem>() : nullptr;
	const ULanguageProgressSubsystem* Progress = GameInstance ? GameInstance->GetSubsystem<ULanguageProgressSubsystem>() : nullptr;

	if (CityText) { CityText->SetText(Pack->CityName); }
	if (LanguageText) { LanguageText->SetText(Pack->LanguageName); }
	if (TaglineText) { TaglineText->SetText(Pack->Tagline); }
	if (ProgressText)
	{
		// Best fluency grades of completed missions, e.g. "Missions 2/2  -  A S"
		TArray<FString> Grades;
		if (Missions)
		{
			for (const TSoftObjectPtr<UMissionAsset>& MissionPtr : Pack->Missions)
			{
				FMissionGrade Best;
				const UMissionAsset* Mission = MissionPtr.LoadSynchronous();
				if (Mission && Missions->GetBestGrade(Mission->MissionId, Best))
				{
					Grades.Add(UFluencyScoring::GradeLetter(Best.Grade).ToString());
				}
			}
		}
		const FText Base = !bPlayable ? LOCTEXT("ComingSoon", "Coming soon")
			: TotalMissions == 0 ? LOCTEXT("NoMissions", "Free roam")
			: FText::Format(LOCTEXT("Progress", "Missions {0}/{1}"), CompletedMissions, TotalMissions);
		ProgressText->SetText(Grades.IsEmpty() ? Base
			: FText::Format(LOCTEXT("ProgressGrades", "{0}  -  grades {1}"), Base, FText::FromString(FString::Join(Grades, TEXT(" ")))));
	}

	const int32 Due = Srs ? Srs->GetDueCount(Pack->CityId) : 0;
	if (DueText)
	{
		DueText->SetText(FText::Format(LOCTEXT("Due", "{0} {0}|plural(one=phrase,other=phrases) to review"), Due));
		DueText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::Danger));
		DueText->SetVisibility(Due > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (WeakSkillText)
	{
		const TArray<FName> Weak = Progress ? Progress->GetWeakSkills(Pack->CityId) : TArray<FName>();
		WeakSkillText->SetText(Weak.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("Weak", "Work on: {0}"), FText::FromName(Weak[0])));
		WeakSkillText->SetVisibility(Weak.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (CardButton)
	{
		CardButton->SetBackgroundColor(Pack->AccentColor);
		CardButton->SetIsEnabled(bPlayable);
	}
	if (PracticeButton)
	{
		PracticeButton->SetIsEnabled(bPlayable && Srs && !Srs->GetSkillStrengths(Pack->CityId).IsEmpty());
		PracticeButton->SetBackgroundColor(Due > 0 ? LinguaSpyStyle::Accent : LinguaSpyStyle::TileNeutral);
	}
	if (PhrasebookButton)
	{
		PhrasebookButton->SetIsEnabled(bPlayable);
	}
}

void UCityCardWidget::HandleClicked()
{
	if (Pack)
	{
		OnCardClicked.Broadcast(Pack);
	}
}

void UCityCardWidget::HandlePractice()
{
	if (Pack)
	{
		OnPracticeClicked.Broadcast(Pack);
	}
}

void UCityCardWidget::HandlePhrasebook()
{
	if (Pack)
	{
		OnPhrasebookClicked.Broadcast(Pack);
	}
}

#undef LOCTEXT_NAMESPACE
