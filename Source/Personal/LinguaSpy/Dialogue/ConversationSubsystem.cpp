// LinguaSpy - runs the active conversation, cover-blown practice drills, Safe house reviews and mission debriefs

#include "ConversationSubsystem.h"
#include "ConversationAsset.h"
#include "CoverComponent.h"
#include "LanguagePackAsset.h"
#include "LanguageProgressSubsystem.h"
#include "LearningTelemetrySubsystem.h"
#include "LinguaSpyAudio.h"
#include "MissionSubsystem.h"
#include "NpcCharacter.h"
#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"
#include "SpacedRepetitionSubsystem.h"
#include "LinguaSpySaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Personal.h"

namespace
{
	template <typename T>
	T* GetGameInstanceSubsystem(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<T>() : nullptr;
	}

	FString ModeName(EConversationMode Mode, bool bDebrief)
	{
		switch (Mode)
		{
		case EConversationMode::Conversation: return TEXT("conversation");
		case EConversationMode::Practice:     return TEXT("drill");
		case EConversationMode::Review:       return bDebrief ? TEXT("debrief") : TEXT("review");
		default:                              return TEXT("other");
		}
	}
}

bool UConversationSubsystem::StartConversation(ANpcCharacter* Npc, UCoverComponent* PlayerCover)
{
	UConversationAsset* Conversation = Npc ? Npc->ResolveConversation() : nullptr;
	return Conversation && BeginConversation(Npc, Conversation, Conversation->StartNode, PlayerCover);
}

bool UConversationSubsystem::StartConversationFromAsset(UConversationAsset* Conversation, FName StartNodeOverride, UCoverComponent* PlayerCover)
{
	if (!Conversation)
	{
		return false;
	}
	const FName Start = (!StartNodeOverride.IsNone() && Conversation->FindNode(StartNodeOverride)) ? StartNodeOverride : Conversation->StartNode;
	return BeginConversation(nullptr, Conversation, Start, PlayerCover);
}

bool UConversationSubsystem::BeginConversation(ANpcCharacter* Npc, UConversationAsset* Conversation, FName StartNode, UCoverComponent* PlayerCover)
{
	if (IsActive() || !Conversation)
	{
		return false;
	}

	ActiveNpc = Npc;
	ActiveConversation = Conversation;
	ActiveCover = PlayerCover;
	FailedItems.Reset();
	PracticeQueue.Reset();
	RevealedNodes.Reset();
	PendingFieldNotes.Reset();
	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	bDebrief = false;
	Mode = EConversationMode::Conversation;

	if (PlayerCover)
	{
		PlayerCover->OnCoverBlown.AddUniqueDynamic(this, &UConversationSubsystem::HandleCoverBlown);
	}

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("conversation_start"))
		.Str(TEXT("conversation"), Conversation->GetName()).Id(TEXT("npc"), Npc ? Npc->GetMissionTargetId() : NAME_None)
		.Id(TEXT("city"), GetCityId()));

	OnConversationStarted.Broadcast(Npc);
	EnterNode(StartNode);
	return true;
}

bool UConversationSubsystem::StartReviewSession(const TArray<FSrsItemKey>& Items, bool bInDebrief)
{
	if (IsActive())
	{
		return false;
	}

	ReviewConversations.Reset();
	PracticeQueue.Reset();
	for (const FSrsItemKey& Key : Items)
	{
		UConversationAsset* Conversation = Cast<UConversationAsset>(Key.Conversation.TryLoad());
		if (Conversation && Conversation->FindChallenge(Key.NodeId, Key.ChoiceIndex))
		{
			ReviewConversations.AddUnique(Conversation);
			PracticeQueue.AddUnique({ Conversation, Key.NodeId, Key.ChoiceIndex });
		}
	}
	if (PracticeQueue.IsEmpty())
	{
		ReviewConversations.Reset();
		return false;
	}

	ActiveNpc.Reset();
	ActiveCover.Reset();
	ActiveConversation = PracticeQueue[0].Conversation;
	FailedItems.Reset();
	ReviewAttempted.Reset();
	RevealedNodes.Reset();
	PendingFieldNotes.Reset();
	ReviewTotal = PracticeQueue.Num();
	ReviewPassed = 0;
	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	bDebrief = bInDebrief;
	Mode = EConversationMode::Review;

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(bDebrief ? TEXT("debrief_start") : TEXT("review_start"))
		.Id(TEXT("city"), GetCityId()).Int(TEXT("items"), ReviewTotal));

	OnConversationStarted.Broadcast(nullptr);
	EnterPracticeItem();
	return true;
}

bool UConversationSubsystem::StartReviewForCity(FName CityId, int32 Max)
{
	const USpacedRepetitionSubsystem* Srs = GetGameInstanceSubsystem<USpacedRepetitionSubsystem>(GetWorld());
	return Srs && StartReviewSession(Srs->BuildReviewQueue(CityId, Max));
}

bool UConversationSubsystem::StartDebrief(const TArray<FSrsItemKey>& Items)
{
	return StartReviewSession(Items, true);
}

void UConversationSubsystem::FinishReview()
{
	const int32 Passed = ReviewPassed;
	const int32 Total = ReviewTotal;
	if (ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld()))
	{
		Save->RequestSave();
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(bDebrief ? TEXT("debrief_end") : TEXT("review_end"))
		.Int(TEXT("passed"), Passed).Int(TEXT("total"), Total));
	OnReviewFinished.Broadcast(Passed, Total);
	EndConversation();
}

// ---- Helper (translations) --------------------------------------------------------------------------------------

bool UConversationSubsystem::RevealTranslation()
{
	if (!IsActive() || CurrentNodeId.IsNone() || RevealedNodes.Contains(CurrentNodeId))
	{
		return false;
	}
	if (GetPeeksLeft() == 0)
	{
		return false;
	}
	RevealedNodes.Add(CurrentNodeId);
	if (Mode == EConversationMode::Conversation)
	{
		++TallyPeeks;
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("peek"))
		.Str(TEXT("conversation"), GetNameSafe(ActiveConversation)).Id(TEXT("node"), CurrentNodeId).Str(TEXT("mode"), ModeName(Mode, bDebrief)));
	return true;
}

bool UConversationSubsystem::IsTranslationRevealed() const
{
	if (const ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld()))
	{
		if (Save->GetHandlerMode(GetCityId()) == EHandlerMode::Shadowing)
		{
			return true;
		}
	}
	return RevealedNodes.Contains(CurrentNodeId);
}

bool UConversationSubsystem::IsPeeked() const
{
	// Shadowing shows translations for free; only a tapped reveal counts
	return RevealedNodes.Contains(CurrentNodeId);
}

int32 UConversationSubsystem::GetPeeksLeft() const
{
	const ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld());
	if (Mode != EConversationMode::Conversation || !Save || Save->GetHandlerMode(GetCityId()) != EHandlerMode::RadioSilence)
	{
		return -1;
	}
	return FMath::Max(RadioSilencePeeks - RevealedNodes.Num(), 0);
}

void UConversationSubsystem::GetFluencyTally(int32& OutChallenges, int32& OutMistakes, int32& OutPeeks) const
{
	OutChallenges = TallyChallenges;
	OutMistakes = TallyMistakes;
	OutPeeks = TallyPeeks;
}

// ---- Voice ------------------------------------------------------------------------------------------------------

bool UConversationSubsystem::HasLineAudio() const
{
	const FConversationNode* Node = GetCurrentNode();
	return Node && !Node->LineAudio.IsNull();
}

void UConversationSubsystem::PlayLine(const FConversationNode& Node, bool bSlow)
{
	if (bSlow && !Node.LineAudioSlow.IsNull())
	{
		LinguaSpyAudio::PlayVoice(this, Node.LineAudioSlow);
	}
	else if (!Node.LineAudio.IsNull())
	{
		// No slow recording: slow the normal one down (lower pitch, but still clearer than full speed)
		LinguaSpyAudio::PlayVoice(this, Node.LineAudio, bSlow ? 0.8f : 1.f);
	}
}

bool UConversationSubsystem::RepeatLine(bool bSlow)
{
	const FConversationNode* Node = GetCurrentNode();
	if (!Node || Node->LineAudio.IsNull())
	{
		return false;
	}
	PlayLine(*Node, bSlow);
	++LineReplays;

	// Asking someone to repeat once is normal; again and again starts to look odd
	if (LineReplays > 1 && Mode == EConversationMode::Conversation)
	{
		if (UCoverComponent* Cover = ActiveCover.Get())
		{
			Cover->ApplyPenalty(RepeatCost);
		}
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(bSlow ? TEXT("slow") : TEXT("repeat"))
		.Str(TEXT("conversation"), GetNameSafe(ActiveConversation)).Id(TEXT("node"), CurrentNodeId).Int(TEXT("count"), LineReplays));
	return true;
}

// ---- Graph ------------------------------------------------------------------------------------------------------

ULanguagePackAsset* UConversationSubsystem::GetActiveLanguage() const
{
	return ActiveConversation ? ActiveConversation->Language.Get() : nullptr;
}

FName UConversationSubsystem::GetCityId() const
{
	const ULanguagePackAsset* Language = GetActiveLanguage();
	return Language ? Language->CityId : FName(TEXT("Unknown"));
}

void UConversationSubsystem::ApplyFlags(const TArray<FName>& Flags) const
{
	if (Flags.IsEmpty())
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld()))
	{
		for (FName Flag : Flags)
		{
			Missions->SetFlag(Flag);
		}
	}
}

void UConversationSubsystem::EnterNode(FName NodeId)
{
	const FConversationNode* Node = ActiveConversation ? ActiveConversation->FindNode(NodeId) : nullptr;
	if (!Node)
	{
		if (!NodeId.IsNone())
		{
			UE_LOG(LogPersonal, Warning, TEXT("Conversation '%s' has no node '%s'"), *GetNameSafe(ActiveConversation), *NodeId.ToString());
		}
		// Running off the end of the graph counts as finishing it
		FinishConversation(LinguaSpyOutcomes::Completed);
		return;
	}

	CurrentNodeId = NodeId;
	ActiveChoice = INDEX_NONE;
	bAwaitingContinueAfterGrade = false;
	bLastAnswerPassed = false;
	LineReplays = 0;
	IntelAttempts = 0;
	Presentation = FChallengePresentation();

	if (Mode == EConversationMode::Conversation)
	{
		ApplyFlags(Node->SetFlags);
	}

	OnNodeEntered.Broadcast(*Node, Mode);
	PlayLine(*Node, false);
	if (Node->Type == EConversationNodeType::PlayerChallenge)
	{
		PresentChallenge(Node->Challenge);
	}
}

bool UConversationSubsystem::IsChoiceAvailable(int32 ChoiceIndex) const
{
	const FConversationNode* Node = GetCurrentNode();
	if (!Node || Node->Type != EConversationNodeType::PlayerChoice || !Node->Choices.IsValidIndex(ChoiceIndex))
	{
		return false;
	}
	const UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld());
	return !Missions || Missions->HasAllFlags(Node->Choices[ChoiceIndex].RequiredFlags);
}

bool UConversationSubsystem::SelectChoice(int32 ChoiceIndex)
{
	const FConversationNode* Node = GetCurrentNode();
	if (bAwaitingContinueAfterGrade || !Node || Node->Type != EConversationNodeType::PlayerChoice || !Node->Choices.IsValidIndex(ChoiceIndex))
	{
		return false;
	}
	ActiveChoice = ChoiceIndex;
	PresentChallenge(Node->Choices[ChoiceIndex].Challenge);
	return true;
}

FSrsItemKey UConversationSubsystem::GetCurrentKey() const
{
	FSrsItemKey Key;
	Key.Conversation = FSoftObjectPath(ActiveConversation);
	Key.NodeId = CurrentNodeId;
	Key.ChoiceIndex = ActiveChoice;
	return Key;
}

void UConversationSubsystem::PresentChallenge(const FWordTileChallenge& Challenge)
{
	const ULanguagePackAsset* Language = GetActiveLanguage();
	const bool bSpaces = !Language || Language->bUsesWordSpacing;
	const FSrsItemKey Key = GetCurrentKey();
	const USpacedRepetitionSubsystem* Srs = GetGameInstanceSubsystem<USpacedRepetitionSubsystem>(GetWorld());

	Presentation = FChallengePresentation();
	Presentation.Challenge = Challenge;
	Presentation.Attempt = 1;
	Presentation.Level = UScaffoldPolicy::ChooseLevel(Srs ? Srs->FindState(Key) : nullptr,
		SessionErrors.FindRef(Key.ToString()), Language && Language->CanType());
	Presentation.Tiles = UScaffoldPolicy::BuildTiles(Challenge, Presentation.Level);
	if (Presentation.Level == EScaffoldLevel::Recognize)
	{
		UScaffoldPolicy::BuildRecognitionOptions(Challenge, bSpaces, GetTypeHash(Key.ToString()) ^ static_cast<uint32>(FDateTime::UtcNow().GetTicks()),
			Presentation.RecognitionOptions, RecognitionCorrect);
		if (Presentation.RecognitionOptions.Num() < 2)
		{
			Presentation.Level = EScaffoldLevel::Assemble; // nothing to choose between
			Presentation.Tiles = UScaffoldPolicy::BuildTiles(Challenge, Presentation.Level);
		}
	}
	if (Presentation.Level == EScaffoldLevel::GapFill)
	{
		Presentation.GapDisplay = UScaffoldPolicy::BuildGapDisplay(Challenge, bSpaces);
		Presentation.GapCount = UScaffoldPolicy::GetGapIndices(Challenge).Num();
	}
	ChallengeStart = FDateTime::UtcNow();
	OnChallengeReady.Broadcast(Presentation);
}

const FConversationNode* UConversationSubsystem::GetCurrentNode() const
{
	return ActiveConversation ? ActiveConversation->FindNode(CurrentNodeId) : nullptr;
}

const FWordTileChallenge* UConversationSubsystem::GetActiveChallenge() const
{
	return ActiveConversation ? ActiveConversation->FindChallenge(CurrentNodeId, ActiveChoice) : nullptr;
}

FString UConversationSubsystem::ModelAnswerFor(const FWordTileChallenge& Challenge) const
{
	if (Challenge.AcceptedAnswers.IsEmpty())
	{
		return FString();
	}
	const ULanguagePackAsset* Language = GetActiveLanguage();
	return USentenceGrader::JoinTokens(Challenge.AcceptedAnswers[0].Tokens, !Language || Language->bUsesWordSpacing);
}

// ---- Answers ----------------------------------------------------------------------------------------------------

bool UConversationSubsystem::CanAnswer() const
{
	return GetActiveChallenge() && !bAwaitingContinueAfterGrade
		&& (Mode == EConversationMode::Conversation || Mode == EConversationMode::Practice || Mode == EConversationMode::Review);
}

FAnswerFeedback UConversationSubsystem::SubmitAnswer(const TArray<FString>& Tokens)
{
	if (!CanAnswer() || Presentation.Level == EScaffoldLevel::Recognize || Presentation.Level == EScaffoldLevel::GapFill)
	{
		return FAnswerFeedback();
	}
	return ApplyGrade(USentenceGrader::Grade(*GetActiveChallenge(), Tokens), Tokens.Num());
}

FAnswerFeedback UConversationSubsystem::SubmitGapWords(const TArray<FString>& Words)
{
	if (!CanAnswer() || Presentation.Level != EScaffoldLevel::GapFill)
	{
		return FAnswerFeedback();
	}
	const FWordTileChallenge& Challenge = *GetActiveChallenge();
	const TArray<FString> Filled = UScaffoldPolicy::FillGaps(Challenge, Words);
	FGradeResult Result = USentenceGrader::Grade(Challenge, Filled);
	return ApplyGrade(Result, Filled.Num());
}

FAnswerFeedback UConversationSubsystem::SubmitRecognition(int32 OptionIndex)
{
	if (!CanAnswer() || Presentation.Level != EScaffoldLevel::Recognize)
	{
		return FAnswerFeedback();
	}
	FGradeResult Result;
	Result.bPassed = OptionIndex == RecognitionCorrect && RecognitionCorrect != INDEX_NONE;
	Result.ClosestAnswerIndex = 0;
	Result.FirstWrongIndex = Result.bPassed ? INDEX_NONE : 0;
	return ApplyGrade(Result, 0);
}

FAnswerFeedback UConversationSubsystem::ApplyGrade(const FGradeResult& Result, int32 SubmittedTokens)
{
	const FWordTileChallenge& Challenge = *GetActiveChallenge();
	const FString KeyString = GetCurrentKey().ToString();
	UCoverComponent* Cover = Mode == EConversationMode::Conversation ? ActiveCover.Get() : nullptr;

	FAnswerFeedback Feedback;
	Feedback.Grade = Result;

	if (!Result.bPassed && Presentation.Attempt < UScaffoldPolicy::MaxAttempts)
	{
		// Feedback ladder step 1: prompt (hint, part of the penalty), and the player tries again
		Feedback.bFinal = false;
		Feedback.Hint = UScaffoldPolicy::ChooseHint(Challenge, Result, SubmittedTokens);
		++Presentation.Attempt;
		++SessionErrors.FindOrAdd(KeyString);
		if (Mode == EConversationMode::Conversation)
		{
			FailedItems.AddUnique({ ActiveConversation, CurrentNodeId, ActiveChoice });
		}
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("prompt"))
			.Str(TEXT("item"), KeyString).Str(TEXT("level"), UScaffoldPolicy::LevelName(Presentation.Level))
			.Bool(TEXT("accent_only"), Result.bAccentOnly).Str(TEXT("mode"), ModeName(Mode, bDebrief)));

		OnAnswerFeedback.Broadcast(Feedback);
		if (Cover)
		{
			Cover->ApplyPenalty(Challenge.CoverPenalty * UScaffoldPolicy::PromptPenaltyShare); // may trigger HandleCoverBlown
		}
		return Feedback;
	}

	// Final: pass (maybe assisted), or the recast with the model answer
	const bool bAssisted = Result.bPassed && (UScaffoldPolicy::IsAssistedPass(Presentation.Level, Presentation.Attempt) || IsPeeked()
		|| Mode == EConversationMode::Practice);
	Feedback.bFinal = true;
	Feedback.bAssisted = bAssisted;
	Feedback.ModelAnswer = ModelAnswerFor(Challenge);
	bLastAnswerPassed = Result.bPassed;
	bAwaitingContinueAfterGrade = true;
	if (!Result.bPassed)
	{
		++SessionErrors.FindOrAdd(KeyString);
	}

	RecordFinal(Challenge, Result.bPassed, bAssisted, Result.bAccentOnly);

	OnAnswerFeedback.Broadcast(Feedback);
	if (!Result.bPassed)
	{
		LinguaSpyAudio::PlayVoice(this, Challenge.ModelAudio);
	}

	if (Mode == EConversationMode::Conversation)
	{
		if (Result.bPassed)
		{
			if (Cover)
			{
				Cover->Recover(CoverRecoveryOnPass);
			}
		}
		else
		{
			FailedItems.AddUnique({ ActiveConversation, CurrentNodeId, ActiveChoice });
			if (Cover)
			{
				const float Share = Presentation.Attempt > 1 ? 1.f - UScaffoldPolicy::PromptPenaltyShare : 1.f;
				Cover->ApplyPenalty(Challenge.CoverPenalty * Share); // may trigger HandleCoverBlown
			}
		}
	}
	return Feedback;
}

void UConversationSubsystem::RecordFinal(const FWordTileChallenge& Challenge, bool bPassed, bool bAssisted, bool bAccentOnly)
{
	const FSrsItemKey Key = GetCurrentKey();

	// Only the first visit of a review item counts (retries later in the same session would inflate its box)
	bool bRecord = true;
	if (Mode == EConversationMode::Review)
	{
		const FConversationPracticeItem Item{ ActiveConversation, CurrentNodeId, ActiveChoice };
		bRecord = !ReviewAttempted.Contains(Item);
		if (bRecord)
		{
			ReviewAttempted.Add(Item);
			ReviewPassed += (bPassed && !bAssisted) ? 1 : 0;
		}
	}
	if (bRecord)
	{
		if (ULanguageProgressSubsystem* Progress = GetGameInstanceSubsystem<ULanguageProgressSubsystem>(GetWorld()))
		{
			Progress->RecordResult(GetCityId(), Challenge.SkillTag, bPassed);
		}
		if (USpacedRepetitionSubsystem* Srs = GetGameInstanceSubsystem<USpacedRepetitionSubsystem>(GetWorld()))
		{
			if (ActiveConversation)
			{
				Srs->RecordResult(Key, GetCityId(), Challenge.SkillTag, bPassed, bAssisted);
			}
		}
	}
	if (Mode == EConversationMode::Conversation)
	{
		++TallyChallenges;
		TallyMistakes += bPassed ? 0 : 1;
		// A pass after a prompt leaned on the handler, like a peek
		TallyPeeks += (bPassed && Presentation.Attempt > 1) ? 1 : 0;

		if (ActiveNpc.IsValid())
		{
			if (UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld()))
			{
				Missions->RecordMissionItem(Key, bPassed, bAssisted);
			}
		}
	}
	if (Mode == EConversationMode::Conversation || Mode == EConversationMode::Review)
	{
		CountFieldNotes(Challenge);
	}

	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("challenge"))
		.Str(TEXT("item"), Key.ToString()).Id(TEXT("city"), GetCityId()).Id(TEXT("skill"), Challenge.SkillTag)
		.Str(TEXT("mode"), ModeName(Mode, bDebrief)).Str(TEXT("level"), UScaffoldPolicy::LevelName(Presentation.Level))
		.Int(TEXT("attempts"), Presentation.Attempt).Bool(TEXT("passed"), bPassed).Bool(TEXT("assisted"), bAssisted)
		.Bool(TEXT("peeked"), IsPeeked()).Bool(TEXT("accent_only"), bAccentOnly).Int(TEXT("replays"), LineReplays)
		.Int(TEXT("ms"), static_cast<int64>((FDateTime::UtcNow() - ChallengeStart).GetTotalMilliseconds()))
		.Bool(TEXT("recorded"), bRecord));
}

void UConversationSubsystem::CountFieldNotes(const FWordTileChallenge& Challenge)
{
	const ULanguagePackAsset* Language = GetActiveLanguage();
	ULinguaSpySaveSubsystem* SaveSubsystem = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld());
	ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	if (!Language || !Save || Challenge.AcceptedAnswers.IsEmpty() || Language->GrammarNotes.IsEmpty())
	{
		return;
	}

	// " je voudrais un café " - padded so triggers match whole words (or whole chunks for unspaced languages)
	TArray<FString> Normalized;
	for (const FString& Token : Challenge.AcceptedAnswers[0].Tokens)
	{
		Normalized.Add(USentenceGrader::NormalizeToken(Token));
	}
	const FString Sep = Language->bUsesWordSpacing ? TEXT(" ") : TEXT("|");
	const FString Haystack = Sep + FString::Join(Normalized, *Sep) + Sep;

	for (const FGrammarNote& Note : Language->GrammarNotes)
	{
		if (Note.NoteId.IsNone() || Save->SeenFieldNotes.Contains(Note.NoteId))
		{
			continue;
		}
		const bool bHit = Note.TriggerTokens.ContainsByPredicate([&Haystack, &Sep](const FString& Trigger)
		{
			return !Trigger.IsEmpty() && Haystack.Contains(Sep + USentenceGrader::NormalizeToken(Trigger) + Sep);
		});
		if (!bHit)
		{
			continue;
		}
		int32& Count = Save->FieldNoteCounts.FindOrAdd(Note.NoteId);
		++Count;
		if (Count >= Note.TriggerAfter)
		{
			Save->SeenFieldNotes.AddUnique(Note.NoteId);
			PendingFieldNotes.AddUnique(Note.NoteId);
		}
	}
}

bool UConversationSubsystem::SubmitIntel(int32 OptionIndex)
{
	const FConversationNode* Node = GetCurrentNode();
	if (!Node || Node->Type != EConversationNodeType::IntelCheck || bAwaitingContinueAfterGrade || Mode != EConversationMode::Conversation)
	{
		return false;
	}
	const bool bCorrect = OptionIndex == Node->Intel.CorrectIndex;
	++IntelAttempts;
	if (IntelAttempts == 1)
	{
		if (ULanguageProgressSubsystem* Progress = GetGameInstanceSubsystem<ULanguageProgressSubsystem>(GetWorld()))
		{
			if (!Node->Intel.SkillTag.IsNone())
			{
				Progress->RecordResult(GetCityId(), Node->Intel.SkillTag, bCorrect);
			}
		}
	}
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("intel_check"))
		.Str(TEXT("conversation"), GetNameSafe(ActiveConversation)).Id(TEXT("node"), CurrentNodeId).Id(TEXT("skill"), Node->Intel.SkillTag)
		.Int(TEXT("attempt"), IntelAttempts).Bool(TEXT("correct"), bCorrect).Int(TEXT("replays"), LineReplays).Bool(TEXT("peeked"), IsPeeked()));

	if (bCorrect)
	{
		bAwaitingContinueAfterGrade = true;
		bLastAnswerPassed = true;
		if (!Node->Intel.IntelFlag.IsNone())
		{
			ApplyFlags({ Node->Intel.IntelFlag });
		}
		OnIntelAnswered.Broadcast(true, OptionIndex);
		if (UCoverComponent* Cover = ActiveCover.Get())
		{
			Cover->Recover(CoverRecoveryOnPass);
		}
	}
	else
	{
		OnIntelAnswered.Broadcast(false, OptionIndex);
		if (UCoverComponent* Cover = ActiveCover.Get())
		{
			Cover->ApplyPenalty(Node->Intel.SuspicionCost); // may trigger HandleCoverBlown
		}
	}
	return bCorrect;
}

// ---- Flow -------------------------------------------------------------------------------------------------------

void UConversationSubsystem::EnterPracticeItem()
{
	if (PracticeQueue.IsEmpty() && Mode == EConversationMode::Review)
	{
		FinishReview();
		return;
	}
	if (PracticeQueue.IsEmpty())
	{
		// Drill complete: restore cover, then either restart or let the mission branch
		if (UCoverComponent* Cover = ActiveCover.Get())
		{
			Cover->ResetCover();
		}
		FailedItems.Reset();
		Mode = EConversationMode::Conversation;
		if (!ActiveConversation->CoverBlownOutcome.IsNone())
		{
			FinishConversation(ActiveConversation->CoverBlownOutcome);
		}
		else
		{
			EnterNode(ActiveConversation->StartNode);
		}
		return;
	}

	const FConversationPracticeItem Item = PracticeQueue[0];
	if (Mode == EConversationMode::Review)
	{
		ActiveConversation = Item.Conversation;
		RevealedNodes.Reset();
	}
	EnterNode(Item.NodeId);
	if (Item.ChoiceIndex != INDEX_NONE)
	{
		SelectChoice(Item.ChoiceIndex);
	}
}

void UConversationSubsystem::HandleCoverBlown()
{
	if (Mode != EConversationMode::Conversation)
	{
		return;
	}
	Mode = EConversationMode::CoverBlown;
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("cover_blown"))
		.Str(TEXT("conversation"), GetNameSafe(ActiveConversation)).Id(TEXT("node"), CurrentNodeId).Int(TEXT("drill_items"), FailedItems.Num()));
	OnCoverBlown.Broadcast();
}

void UConversationSubsystem::Continue()
{
	switch (Mode)
	{
	case EConversationMode::None:
		return;

	case EConversationMode::CoverBlown:
		// (Empty when blown by intel checks / repeats only: the drill finishes straight away)
		PracticeQueue = FailedItems;
		Mode = EConversationMode::Practice;
		EnterPracticeItem();
		return;

	case EConversationMode::Practice:
	case EConversationMode::Review:
		if (bAwaitingContinueAfterGrade && PracticeQueue.Num() > 0)
		{
			const FConversationPracticeItem Item = PracticeQueue[0];
			PracticeQueue.RemoveAt(0);
			if (!bLastAnswerPassed)
			{
				PracticeQueue.Add(Item); // try again later in the drill
			}
			EnterPracticeItem();
		}
		return;

	case EConversationMode::Conversation:
	{
		const FConversationNode* Node = GetCurrentNode();
		if (!Node)
		{
			EndConversation();
			return;
		}
		switch (Node->Type)
		{
		case EConversationNodeType::NpcLine:
			EnterNode(Node->NextNode);
			break;

		case EConversationNodeType::End:
			FinishConversation(Node->Outcome.IsNone() ? LinguaSpyOutcomes::Completed : Node->Outcome);
			break;

		case EConversationNodeType::IntelCheck:
			if (bAwaitingContinueAfterGrade)
			{
				EnterNode(Node->NextNode);
			}
			break;

		case EConversationNodeType::PlayerChallenge:
			if (bAwaitingContinueAfterGrade)
			{
				EnterNode(bLastAnswerPassed ? Node->NextNode : (Node->FailNode.IsNone() ? CurrentNodeId : Node->FailNode));
			}
			break;

		case EConversationNodeType::PlayerChoice:
			if (bAwaitingContinueAfterGrade && Node->Choices.IsValidIndex(ActiveChoice))
			{
				const FConversationChoice& Choice = Node->Choices[ActiveChoice];
				if (bLastAnswerPassed)
				{
					ApplyFlags(Choice.SetFlags);
					EnterNode(Choice.NextNode);
				}
				else
				{
					// Back to the choice so the player can try again or pick another approach
					EnterNode(Choice.FailNode.IsNone() ? CurrentNodeId : Choice.FailNode);
				}
			}
			break;
		}
		return;
	}
	}
}

void UConversationSubsystem::FinishConversation(FName Outcome)
{
	const FName NpcTargetId = ActiveNpc.IsValid() ? ActiveNpc->GetMissionTargetId() : NAME_None;
	ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("conversation_end"))
		.Str(TEXT("conversation"), GetNameSafe(ActiveConversation)).Id(TEXT("outcome"), Outcome)
		.Int(TEXT("challenges"), TallyChallenges).Int(TEXT("mistakes"), TallyMistakes).Int(TEXT("peeks"), TallyPeeks));
	EndConversation();

	// Debug conversations (no NPC) don't affect missions
	if (NpcTargetId.IsNone())
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld()))
	{
		Missions->ReportConversationFinished(NpcTargetId, Outcome);
	}
}

void UConversationSubsystem::EndConversation()
{
	if (!IsActive())
	{
		return;
	}
	if (UCoverComponent* Cover = ActiveCover.Get())
	{
		Cover->OnCoverBlown.RemoveDynamic(this, &UConversationSubsystem::HandleCoverBlown);
	}
	LinguaSpyAudio::StopVoice(this);

	// Conversations with a real NPC count towards the active mission's fluency grade (side chats and walking away too)
	if (ActiveNpc.IsValid() && (TallyChallenges > 0 || TallyPeeks > 0))
	{
		if (UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld()))
		{
			Missions->AddFluencyTally(TallyChallenges, TallyMistakes, TallyPeeks);
		}
	}

	// Field notes unlocked during the conversation (resolved before the language pointer goes away)
	TArray<FGrammarNote> Notes;
	if (const ULanguagePackAsset* Language = GetActiveLanguage())
	{
		for (const FName NoteId : PendingFieldNotes)
		{
			if (const FGrammarNote* Note = Language->GrammarNotes.FindByPredicate([NoteId](const FGrammarNote& N) { return N.NoteId == NoteId; }))
			{
				Notes.Add(*Note);
			}
		}
	}
	PendingFieldNotes.Reset();

	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	RevealedNodes.Reset();
	ReviewConversations.Reset();
	ReviewAttempted.Reset();
	ReviewTotal = ReviewPassed = 0;
	bDebrief = false;
	Mode = EConversationMode::None;
	ActiveConversation = nullptr;
	ActiveNpc.Reset();
	ActiveCover.Reset();
	CurrentNodeId = NAME_None;
	ActiveChoice = INDEX_NONE;
	Presentation = FChallengePresentation();
	PracticeQueue.Reset();
	FailedItems.Reset();
	OnConversationEnded.Broadcast();

	for (const FGrammarNote& Note : Notes)
	{
		ULearningTelemetrySubsystem::LogEvent(this, FTelemetryEvent(TEXT("field_note")).Id(TEXT("note"), Note.NoteId));
		OnFieldNoteUnlocked.Broadcast(Note);
	}
	if (!Notes.IsEmpty())
	{
		if (ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld()))
		{
			Save->RequestSave();
		}
	}
}

void UConversationSubsystem::Deinitialize()
{
	EndConversation();
	Super::Deinitialize();
}
