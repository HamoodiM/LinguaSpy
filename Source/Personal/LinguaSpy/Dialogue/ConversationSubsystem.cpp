// LinguaSpy - runs the active conversation and cover-blown practice drills

#include "ConversationSubsystem.h"
#include "ConversationAsset.h"
#include "CoverComponent.h"
#include "LanguagePackAsset.h"
#include "LanguageProgressSubsystem.h"
#include "MissionSubsystem.h"
#include "NpcCharacter.h"
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
	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	Mode = EConversationMode::Conversation;

	if (PlayerCover)
	{
		PlayerCover->OnCoverBlown.AddUniqueDynamic(this, &UConversationSubsystem::HandleCoverBlown);
	}

	OnConversationStarted.Broadcast(Npc);
	EnterNode(StartNode);
	return true;
}

bool UConversationSubsystem::StartReviewSession(const TArray<FSrsItemKey>& Items)
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
	ReviewTotal = PracticeQueue.Num();
	ReviewPassed = 0;
	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	Mode = EConversationMode::Review;

	OnConversationStarted.Broadcast(nullptr);
	EnterPracticeItem();
	return true;
}

bool UConversationSubsystem::StartReviewForCity(FName CityId, int32 Max)
{
	const USpacedRepetitionSubsystem* Srs = GetGameInstanceSubsystem<USpacedRepetitionSubsystem>(GetWorld());
	return Srs && StartReviewSession(Srs->BuildReviewQueue(CityId, Max));
}

void UConversationSubsystem::FinishReview()
{
	const int32 Passed = ReviewPassed;
	const int32 Total = ReviewTotal;
	if (ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld()))
	{
		Save->RequestSave();
	}
	OnReviewFinished.Broadcast(Passed, Total);
	EndConversation();
}

void UConversationSubsystem::RevealTranslation()
{
	if (!IsActive() || CurrentNodeId.IsNone() || RevealedNodes.Contains(CurrentNodeId))
	{
		return;
	}
	RevealedNodes.Add(CurrentNodeId);
	if (Mode == EConversationMode::Conversation)
	{
		++TallyPeeks;
	}
}

bool UConversationSubsystem::IsTranslationRevealed() const
{
	if (const ULinguaSpySaveSubsystem* Save = GetGameInstanceSubsystem<ULinguaSpySaveSubsystem>(GetWorld()))
	{
		if (Save->GetSettings().bAlwaysShowTranslation)
		{
			return true;
		}
	}
	return RevealedNodes.Contains(CurrentNodeId);
}

void UConversationSubsystem::GetFluencyTally(int32& OutChallenges, int32& OutMistakes, int32& OutPeeks) const
{
	OutChallenges = TallyChallenges;
	OutMistakes = TallyMistakes;
	OutPeeks = TallyPeeks;
}

bool UConversationSubsystem::IsAssistedAnswer() const
{
	// After a cover-blown drill the player has already been shown the model answer
	return Mode == EConversationMode::Practice || IsTranslationRevealed();
}

void UConversationSubsystem::RecordSrs(const FWordTileChallenge& Challenge, bool bPassed)
{
	USpacedRepetitionSubsystem* Srs = GetGameInstanceSubsystem<USpacedRepetitionSubsystem>(GetWorld());
	if (!Srs || !ActiveConversation)
	{
		return;
	}
	FSrsItemKey Key;
	Key.Conversation = FSoftObjectPath(ActiveConversation);
	Key.NodeId = CurrentNodeId;
	Key.ChoiceIndex = ActiveChoice;
	Srs->RecordResult(Key, GetCityId(), Challenge.SkillTag, bPassed, IsAssistedAnswer());
}

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

	if (Mode == EConversationMode::Conversation)
	{
		ApplyFlags(Node->SetFlags);
	}

	OnNodeEntered.Broadcast(*Node, Mode);
	if (Node->Type == EConversationNodeType::PlayerChallenge)
	{
		OnChallengeReady.Broadcast(Node->Challenge);
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
	OnChallengeReady.Broadcast(Node->Choices[ChoiceIndex].Challenge);
	return true;
}

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

const FConversationNode* UConversationSubsystem::GetCurrentNode() const
{
	return ActiveConversation ? ActiveConversation->FindNode(CurrentNodeId) : nullptr;
}

const FWordTileChallenge* UConversationSubsystem::GetActiveChallenge() const
{
	return ActiveConversation ? ActiveConversation->FindChallenge(CurrentNodeId, ActiveChoice) : nullptr;
}

FGradeResult UConversationSubsystem::SubmitAnswer(const TArray<FString>& Tokens)
{
	const FWordTileChallenge* Challenge = GetActiveChallenge();
	if (!Challenge || bAwaitingContinueAfterGrade
		|| (Mode != EConversationMode::Conversation && Mode != EConversationMode::Practice && Mode != EConversationMode::Review))
	{
		return FGradeResult();
	}

	const FGradeResult Result = USentenceGrader::Grade(*Challenge, Tokens);
	bLastAnswerPassed = Result.bPassed;
	bAwaitingContinueAfterGrade = true;

	// Only the first attempt at a review item counts (retries later in the same session would inflate its box)
	bool bRecord = true;
	if (Mode == EConversationMode::Review)
	{
		const FConversationPracticeItem Item{ ActiveConversation, CurrentNodeId, ActiveChoice };
		bRecord = !ReviewAttempted.Contains(Item);
		if (bRecord)
		{
			ReviewAttempted.Add(Item);
			ReviewPassed += Result.bPassed ? 1 : 0;
		}
	}
	if (bRecord)
	{
		if (ULanguageProgressSubsystem* Progress = GetGameInstanceSubsystem<ULanguageProgressSubsystem>(GetWorld()))
		{
			Progress->RecordResult(GetCityId(), Challenge->SkillTag, Result.bPassed);
		}
		RecordSrs(*Challenge, Result.bPassed);
	}
	if (Mode == EConversationMode::Conversation)
	{
		++TallyChallenges;
		TallyMistakes += Result.bPassed ? 0 : 1;
	}

	FString ModelAnswer;
	if (Challenge->AcceptedAnswers.Num() > 0)
	{
		const ULanguagePackAsset* Language = GetActiveLanguage();
		ModelAnswer = USentenceGrader::JoinTokens(Challenge->AcceptedAnswers[0].Tokens, !Language || Language->bUsesWordSpacing);
	}

	// Broadcast before applying the penalty so the UI shows the grade, then the cover-blown state
	OnAnswerGraded.Broadcast(Result, ModelAnswer);

	if (Mode == EConversationMode::Conversation)
	{
		UCoverComponent* Cover = ActiveCover.Get();
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
				Cover->ApplyPenalty(Challenge->CoverPenalty); // may trigger HandleCoverBlown
			}
		}
	}

	return Result;
}

void UConversationSubsystem::HandleCoverBlown()
{
	if (Mode != EConversationMode::Conversation)
	{
		return;
	}
	Mode = EConversationMode::CoverBlown;
	OnCoverBlown.Broadcast();
}

void UConversationSubsystem::Continue()
{
	switch (Mode)
	{
	case EConversationMode::None:
		return;

	case EConversationMode::CoverBlown:
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

	// Conversations with a real NPC count towards the active mission's fluency grade (side chats and walking away too)
	if (ActiveNpc.IsValid() && (TallyChallenges > 0 || TallyPeeks > 0))
	{
		if (UMissionSubsystem* Missions = GetGameInstanceSubsystem<UMissionSubsystem>(GetWorld()))
		{
			Missions->AddFluencyTally(TallyChallenges, TallyMistakes, TallyPeeks);
		}
	}
	TallyChallenges = TallyMistakes = TallyPeeks = 0;
	RevealedNodes.Reset();
	ReviewConversations.Reset();
	ReviewAttempted.Reset();
	ReviewTotal = ReviewPassed = 0;
	Mode = EConversationMode::None;
	ActiveConversation = nullptr;
	ActiveNpc.Reset();
	ActiveCover.Reset();
	CurrentNodeId = NAME_None;
	ActiveChoice = INDEX_NONE;
	PracticeQueue.Reset();
	FailedItems.Reset();
	OnConversationEnded.Broadcast();
}

void UConversationSubsystem::Deinitialize()
{
	EndConversation();
	Super::Deinitialize();
}
