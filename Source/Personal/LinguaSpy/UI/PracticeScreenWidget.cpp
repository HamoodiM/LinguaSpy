// LinguaSpy - "Safe house" overlay: spaced-repetition review outside missions

#include "PracticeScreenWidget.h"
#include "ConversationSubsystem.h"
#include "ConversationWidget.h"
#include "LanguagePackAsset.h"
#include "LanguageProgressSubsystem.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "SpacedRepetitionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "LinguaSpyPractice"

FText UPracticeScreenWidget::GetTitle() const
{
	return LOCTEXT("Title", "SAFE HOUSE");
}

UConversationSubsystem* UPracticeScreenWidget::GetConversations() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UConversationSubsystem>() : nullptr;
}

void UPracticeScreenWidget::BuildContent()
{
	using namespace LinguaSpyStyle;

	AddRow(ContentBox, MakeText(LOCTEXT("Intro",
		"Your handler runs you through the phrases you've used in the field. No cover to lose here - but phrases you peeked at or missed come back sooner."),
		17, TextMuted), 0.f);

	StatusText = MakeText(FText::GetEmpty(), 20, TextMain, true);
	AddRow(ContentBox, StatusText, 16.f);

	SkillBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SkillBox"));
	AddRow(ContentBox, SkillBox, 10.f);

	ResultText = MakeText(FText::GetEmpty(), 19, TileCorrect, true);
	AddRow(ContentBox, ResultText, 16.f);

	UTextBlock* Label = nullptr;
	StartButton = MakeButton(FText::GetEmpty(), &Label, 20);
	StartLabel = Label;
	StartButton->OnClicked.AddDynamic(this, &UPracticeScreenWidget::HandleStart);
	AddRow(ContentBox, StartButton, 18.f);
}

void UPracticeScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UConversationSubsystem* Conversations = GetConversations())
	{
		Conversations->OnReviewFinished.AddUniqueDynamic(this, &UPracticeScreenWidget::HandleReviewFinished);
		Conversations->OnConversationEnded.AddUniqueDynamic(this, &UPracticeScreenWidget::HandleConversationEnded);
	}
	if (!DrillWidget && GetOwningPlayer())
	{
		DrillWidget = CreateWidget<UConversationWidget>(GetOwningPlayer(), UConversationWidget::StaticClass());
		DrillWidget->AddToViewport(130);
	}
}

void UPracticeScreenWidget::NativeDestruct()
{
	if (UConversationSubsystem* Conversations = GetConversations())
	{
		Conversations->OnReviewFinished.RemoveAll(this);
		Conversations->OnConversationEnded.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UPracticeScreenWidget::SetPack(ULanguagePackAsset* InPack)
{
	Pack = InPack;
	if (Pack)
	{
		SetTitle(FText::Format(LOCTEXT("TitleCity", "SAFE HOUSE - {0}"), Pack->CityName));
	}
	if (ResultText)
	{
		ResultText->SetText(FText::GetEmpty());
	}
	Refresh();
}

void UPracticeScreenWidget::Refresh()
{
	using namespace LinguaSpyStyle;

	const UGameInstance* GameInstance = GetGameInstance();
	const USpacedRepetitionSubsystem* Srs = GameInstance ? GameInstance->GetSubsystem<USpacedRepetitionSubsystem>() : nullptr;
	const ULanguageProgressSubsystem* Progress = GameInstance ? GameInstance->GetSubsystem<ULanguageProgressSubsystem>() : nullptr;
	if (!Pack || !Srs)
	{
		return;
	}
	const FName City = Pack->CityId;
	const int32 Due = Srs->GetDueCount(City);
	const TMap<FName, float> Strengths = Srs->GetSkillStrengths(City);

	if (StatusText)
	{
		StatusText->SetText(Strengths.IsEmpty()
			? LOCTEXT("NothingYet", "Nothing to drill yet - talk to some locals on a mission first.")
			: Due > 0
				? FText::Format(LOCTEXT("DueCount", "{0} {0}|plural(one=phrase is,other=phrases are) due for review."), Due)
				: LOCTEXT("NoneDue", "All caught up. Extra practice drills your weakest phrases."));
	}

	if (SkillBox)
	{
		SkillBox->ClearChildren();
		const TArray<FName> Weak = Progress ? Progress->GetWeakSkills(City) : TArray<FName>();
		for (const TPair<FName, float>& Pair : Strengths)
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			NameSize->SetWidthOverride(170.f);
			const bool bWeak = Weak.Contains(Pair.Key);
			NameSize->SetContent(MakeText(bWeak ? FText::Format(LOCTEXT("WeakSkill", "{0} (weak)"), FText::FromName(Pair.Key)) : FText::FromName(Pair.Key),
				17, bWeak ? TileWrong : TextMain, true));
			Row->AddChildToHorizontalBox(NameSize);

			USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BarSize->SetHeightOverride(14.f);
			BarSize->SetContent(MakeBar(FMath::Max(Pair.Value, 0.03f), Pair.Value >= 0.6f ? TileCorrect : Pair.Value >= 0.3f ? Accent : TileWrong, 14.f));
			if (UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(BarSize))
			{
				BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				BarSlot->SetVerticalAlignment(VAlign_Center);
			}
			AddRow(SkillBox, Row, 8.f);
		}
	}

	if (StartButton && StartLabel)
	{
		StartButton->SetIsEnabled(!Strengths.IsEmpty());
		StartLabel->SetText(Due > 0
			? FText::Format(LOCTEXT("StartReview", "Review {0} due"), FMath::Min(Due, SessionSize))
			: LOCTEXT("ExtraPractice", "Extra practice"));
	}
}

void UPracticeScreenWidget::HandleStart()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::MenuSelect);
	UConversationSubsystem* Conversations = GetConversations();
	if (Pack && Conversations && Conversations->StartReviewForCity(Pack->CityId, SessionSize))
	{
		if (ResultText)
		{
			ResultText->SetText(FText::GetEmpty());
		}
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (ResultText)
	{
		ResultText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::TextMuted));
		ResultText->SetText(LOCTEXT("NoItems", "Nothing to review yet."));
	}
}

void UPracticeScreenWidget::HandleReviewFinished(int32 Passed, int32 Total)
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::MissionComplete);
	if (ResultText)
	{
		ResultText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::TileCorrect));
		ResultText->SetText(FText::Format(LOCTEXT("Result", "Session done: {0} of {1} right on the first try."), Passed, Total));
	}
}

void UPracticeScreenWidget::HandleConversationEnded()
{
	// Back from the drill (finished or "walk away")
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

void UPracticeScreenWidget::Close()
{
	if (UConversationSubsystem* Conversations = GetConversations())
	{
		if (Conversations->GetMode() == EConversationMode::Review)
		{
			Conversations->EndConversation();
		}
	}
	if (DrillWidget)
	{
		DrillWidget->RemoveFromParent();
		DrillWidget = nullptr;
	}
	Super::Close();
}

#undef LOCTEXT_NAMESPACE
