// LinguaSpy - mission briefing: meet the mission's phrases before you have to say them

#include "BriefingWidget.h"
#include "LanguagePackAsset.h"
#include "LinguaSpyAudio.h"
#include "LinguaSpyStyle.h"
#include "LearningTelemetrySubsystem.h"
#include "MissionAsset.h"
#include "MissionSubsystem.h"
#include "WordTileWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpyBriefing"

FText UBriefingWidget::GetTitle() const
{
	return LOCTEXT("Title", "MISSION BRIEFING");
}

void UBriefingWidget::BuildContent()
{
	using namespace LinguaSpyStyle;

	StepText = MakeText(FText::GetEmpty(), 15, TextMuted);
	AddRow(ContentBox, StepText, 0.f);

	Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddRow(ContentBox, Body, 12.f);

	FeedbackLine = MakeText(FText::GetEmpty(), 17, TextHelper);
	AddRow(ContentBox, FeedbackLine, 14.f);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	HearButton = MakeButton(LOCTEXT("Hear", "Hear it"));
	HearButton->OnClicked.AddDynamic(this, &UBriefingWidget::HandleHear);
	Buttons->AddChildToHorizontalBox(HearButton);
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* SpacerSlot = Buttons->AddChildToHorizontalBox(Spacer))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UTextBlock* Label = nullptr;
	NextButton = MakeButton(LOCTEXT("Next", "Next"), &Label);
	NextLabel = Label;
	NextButton->OnClicked.AddDynamic(this, &UBriefingWidget::HandleNext);
	Buttons->AddChildToHorizontalBox(NextButton);
	AddRow(ContentBox, Buttons, 18.f);
}

void UBriefingWidget::SetMission(UMissionAsset* InMission, ULanguagePackAsset* InLanguage, bool bInReviewOnly)
{
	Mission = InMission;
	Language = InLanguage;
	bReviewOnly = bInReviewOnly;
	StudyIndex = 0;
	QuizIndex = INDEX_NONE;
	if (Mission)
	{
		SetTitle(FText::Format(LOCTEXT("TitleMission", "BRIEFING: {0}"), Mission->Title));
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("briefing_start"))
			.Id(TEXT("mission"), Mission->MissionId).Int(TEXT("chunks"), Mission->BriefingChunks.Num()).Bool(TEXT("review"), bReviewOnly));
	}
	ShowStudyCard();
}

void UBriefingWidget::ClearBody()
{
	Options.Reset();
	if (Body)
	{
		Body->ClearChildren();
	}
	if (FeedbackLine)
	{
		FeedbackLine->SetText(FText::GetEmpty());
	}
}

void UBriefingWidget::PlayChunk(int32 Index) const
{
	if (Mission && Mission->BriefingChunks.IsValidIndex(Index))
	{
		LinguaSpyAudio::PlayVoice(this, Mission->BriefingChunks[Index].Audio);
	}
}

void UBriefingWidget::ShowStudyCard()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	if (!Mission || !Mission->BriefingChunks.IsValidIndex(StudyIndex))
	{
		BuildQuiz();
		ShowQuizItem();
		return;
	}
	const FBriefingChunk& Chunk = Mission->BriefingChunks[StudyIndex];

	if (StepText)
	{
		StepText->SetText(FText::Format(LOCTEXT("StudyStep", "Your cover phrases ({0} / {1}). Listen, read, and picture when you'd use it."),
			StudyIndex + 1, Mission->BriefingChunks.Num()));
	}
	if (StudyIndex == 0 && Body)
	{
		AddRow(Body, MakeText(Mission->Briefing, 17, TextMain), 0.f);
	}

	UTextBlock* Phrase = MakeText(FText::FromString(Chunk.Text), 40, Accent, true);
	if (Language && !Language->NativeFont.IsNull())
	{
		if (UFont* Font = Language->NativeFont.LoadSynchronous())
		{
			FSlateFontInfo Info = Phrase->GetFont();
			Info.FontObject = Font;
			Info.TypefaceFontName = NAME_None;
			Phrase->SetFont(Info);
		}
	}
	AddRow(Body, Phrase, 14.f);
	if (!Chunk.Reading.IsEmpty())
	{
		AddRow(Body, MakeText(FText::FromString(Chunk.Reading), 18, TextMuted), 2.f);
	}
	AddRow(Body, MakeText(FText::Format(LOCTEXT("Means", "means: {0}"), Chunk.Meaning), 24, TextHelper, true), 8.f);
	if (!Chunk.Situation.IsEmpty())
	{
		AddRow(Body, MakeText(FText::Format(LOCTEXT("Use", "Use it when: {0}"), Chunk.Situation), 16, TextMuted), 8.f);
	}

	HearChunk = StudyIndex;
	if (HearButton)
	{
		HearButton->SetVisibility(Chunk.Audio.IsNull() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (NextLabel)
	{
		NextLabel->SetText(StudyIndex + 1 < Mission->BriefingChunks.Num() ? LOCTEXT("NextPhrase", "Next phrase") : LOCTEXT("ToQuiz", "Quick check"));
	}
	PlayChunk(StudyIndex);
}

void UBriefingWidget::BuildQuiz()
{
	Quiz.Reset();
	if (!Mission)
	{
		return;
	}
	const int32 Count = Mission->BriefingChunks.Num();
	TArray<int32> Order;
	for (int32 i = 0; i < Count; ++i)
	{
		Order.Add(i);
	}
	for (int32 i = Order.Num() - 1; i > 0; --i)
	{
		Order.Swap(i, FMath::RandRange(0, i));
	}
	// Round 1: meaning (listen when there's audio, alternating with reading so both get practice)
	for (int32 i = 0; i < Order.Num(); ++i)
	{
		const bool bAudio = !Mission->BriefingChunks[Order[i]].Audio.IsNull();
		Quiz.Add({ Order[i], (bAudio && i % 2 == 0) ? EQuizKind::Listen : EQuizKind::Read, false });
	}
	// Round 2: "which would you say?" (only chunks with a situation)
	for (int32 i = Order.Num() - 1; i >= 0; --i)
	{
		if (!Mission->BriefingChunks[Order[i]].Situation.IsEmpty())
		{
			Quiz.Add({ Order[i], EQuizKind::Situation, false });
		}
	}
	QuizIndex = 0;
}

UWordTileWidget* UBriefingWidget::AddOption(const FString& Label, bool bNative)
{
	UWordTileWidget* Option = CreateWidget<UWordTileWidget>(GetOwningPlayer(), UWordTileWidget::StaticClass());
	Option->SetWord(Label);
	Option->SetChoiceStyle();
	if (bNative && Language && !Language->NativeFont.IsNull())
	{
		Option->SetNativeFont(Language->NativeFont.LoadSynchronous());
	}
	Option->OnTileClicked.AddDynamic(this, &UBriefingWidget::HandleOption);
	AddRow(Body, Option, 8.f);
	return Option;
}

void UBriefingWidget::ShowQuizItem()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	bAnswered = false;
	if (!Mission || !Quiz.IsValidIndex(QuizIndex))
	{
		ShowSummary();
		return;
	}
	const FQuizItem& Item = Quiz[QuizIndex];
	const TArray<FBriefingChunk>& Chunks = Mission->BriefingChunks;
	const FBriefingChunk& Chunk = Chunks[Item.Chunk];

	if (StepText)
	{
		StepText->SetText(FText::Format(LOCTEXT("QuizStep", "Quick check ({0} / {1}) - nothing at stake, your cover is safe here."), QuizIndex + 1, Quiz.Num()));
	}

	// Two distractors from the other chunks
	TArray<int32> Others;
	for (int32 i = 0; i < Chunks.Num(); ++i)
	{
		if (i != Item.Chunk)
		{
			Others.Add(i);
		}
	}
	for (int32 i = Others.Num() - 1; i > 0; --i)
	{
		Others.Swap(i, FMath::RandRange(0, i));
	}
	TArray<int32> Picks = { Item.Chunk };
	for (int32 i = 0; i < Others.Num() && Picks.Num() < 3; ++i)
	{
		Picks.Add(Others[i]);
	}
	for (int32 i = Picks.Num() - 1; i > 0; --i)
	{
		Picks.Swap(i, FMath::RandRange(0, i));
	}
	CorrectOption = Picks.IndexOfByKey(Item.Chunk);

	HearChunk = Item.Chunk;
	switch (Item.Kind)
	{
	case EQuizKind::Listen:
		AddRow(Body, MakeText(LOCTEXT("ListenQ", "Listen. What does it mean?"), 22, TextMain, true), 0.f);
		PlayChunk(Item.Chunk);
		break;
	case EQuizKind::Read:
		AddRow(Body, MakeText(LOCTEXT("ReadQ", "What does this mean?"), 22, TextMain, true), 0.f);
		AddRow(Body, MakeText(FText::FromString(Chunk.Text), 34, Accent, true), 8.f);
		break;
	case EQuizKind::Situation:
		AddRow(Body, MakeText(FText::Format(LOCTEXT("SituationQ", "Which would you say? {0}"), Chunk.Situation), 22, TextMain, true), 0.f);
		break;
	}

	for (int32 i = 0; i < Picks.Num(); ++i)
	{
		const FBriefingChunk& Pick = Chunks[Picks[i]];
		const bool bPhrase = Item.Kind == EQuizKind::Situation;
		UWordTileWidget* Option = AddOption(bPhrase ? Pick.Text : Pick.Meaning.ToString(), bPhrase);
		Options.Add(Option, i);
	}

	if (HearButton)
	{
		HearButton->SetVisibility(Item.Kind == EQuizKind::Listen && !Chunk.Audio.IsNull() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (NextButton)
	{
		NextButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBriefingWidget::HandleOption(UWordTileWidget* Option)
{
	const int32* Index = Options.Find(Option);
	if (!Index || bAnswered || !Mission || !Quiz.IsValidIndex(QuizIndex))
	{
		return;
	}
	bAnswered = true;
	const FQuizItem Item = Quiz[QuizIndex];
	const bool bCorrect = *Index == CorrectOption;
	LinguaSpyAudio::PlayUI(this, bCorrect ? ESpyUISound::Pass : ESpyUISound::Fail);

	for (const TPair<TObjectPtr<UWordTileWidget>, int32>& Pair : Options)
	{
		Pair.Key->SetLocked(true);
		if (Pair.Value == CorrectOption)
		{
			Pair.Key->SetTileState(EWordTileState::Correct);
		}
	}
	if (!bCorrect)
	{
		Option->SetTileState(EWordTileState::Wrong);
		Option->PlayShake();
	}

	if (!Item.bRetry)
	{
		++FirstTryTotal;
		FirstTryCorrect += bCorrect ? 1 : 0;
	}
	const FBriefingChunk& Chunk = Mission->BriefingChunks[Item.Chunk];
	if (FeedbackLine)
	{
		FeedbackLine->SetText(bCorrect
			? FText::Format(LOCTEXT("Right", "Right: \"{0}\" = {1}"), FText::FromString(Chunk.Text), Chunk.Meaning)
			: FText::Format(LOCTEXT("Wrong", "Remember: \"{0}\" = {1}. It'll come up again."), FText::FromString(Chunk.Text), Chunk.Meaning));
	}
	if (!bCorrect && !Item.bRetry)
	{
		Quiz.Add({ Item.Chunk, Item.Kind, true });
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("briefing_item"))
		.Id(TEXT("mission"), Mission->MissionId).Str(TEXT("chunk"), Chunk.Text).Id(TEXT("skill"), Chunk.SkillTag)
		.Str(TEXT("kind"), Item.Kind == EQuizKind::Listen ? TEXT("listen") : Item.Kind == EQuizKind::Read ? TEXT("read") : TEXT("situation"))
		.Bool(TEXT("correct"), bCorrect).Bool(TEXT("retry"), Item.bRetry));

	if (NextButton)
	{
		NextButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (NextLabel)
	{
		NextLabel->SetText(LOCTEXT("Next", "Next"));
	}
	PlayChunk(Item.Chunk);
}

void UBriefingWidget::ShowSummary()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	bDone = true;
	if (StepText)
	{
		StepText->SetText(LOCTEXT("SummaryStep", "Briefing complete."));
	}
	AddRow(Body, MakeText(FText::Format(LOCTEXT("Summary", "{0} of {1} right on the first try."), FirstTryCorrect, FirstTryTotal), 26, Accent, true), 0.f);
	AddRow(Body, MakeText(LOCTEXT("SummaryHint",
		"In the field you'll build these phrases yourself. Your handler starts by giving you just the words you need, "
		"and steps back as you get better. Stuck? Try again after a hint, or tap Translate."), 17, TextMain), 10.f);
	if (Mission && !Mission->CanDoStatements.IsEmpty())
	{
		AddRow(Body, MakeText(LOCTEXT("Goals", "Mission goals:"), 18, TextHelper, true), 14.f);
		for (const FText& CanDo : Mission->CanDoStatements)
		{
			AddRow(Body, MakeText(FText::Format(LOCTEXT("Goal", "- {0}"), CanDo), 16, TextMain), 4.f);
		}
	}
	if (HearButton)
	{
		HearButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (NextButton)
	{
		NextButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (NextLabel)
	{
		NextLabel->SetText(bReviewOnly ? LOCTEXT("Back", "Back") : LOCTEXT("Start", "Start the mission"));
	}
}

void UBriefingWidget::HandleNext()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (bDone)
	{
		Close();
		return;
	}
	if (QuizIndex == INDEX_NONE)
	{
		++StudyIndex;
		ShowStudyCard();
		return;
	}
	++QuizIndex;
	ShowQuizItem();
}

void UBriefingWidget::HandleHear()
{
	PlayChunk(HearChunk);
}

void UBriefingWidget::Close()
{
	LinguaSpyAudio::StopVoice(this);
	if (Mission && !bReviewOnly)
	{
		if (UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr)
		{
			Missions->MarkBriefingCompleted(Mission, FirstTryCorrect, FirstTryTotal);
		}
	}
	Super::Close();
}

#undef LOCTEXT_NAMESPACE
