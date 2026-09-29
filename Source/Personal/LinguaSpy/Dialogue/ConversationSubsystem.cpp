// LinguaSpy - runs the active conversation and cover-blown practice drills

#include "ConversationSubsystem.h"
#include "ConversationAsset.h"
#include "CoverComponent.h"
#include "LanguagePackAsset.h"
#include "LanguageProgressSubsystem.h"
#include "MissionSubsystem.h"
#include "NpcCharacter.h"
#include "SentenceGrader.h"
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
	Mode = EConversationMode::Conversation;

	if (PlayerCover)
	{
		PlayerCover->OnCoverBlown.AddUniqueDynamic(this, &UConversationSubsystem::HandleCoverBlown);
	}

	OnConversationStarted.Broadcast(Npc);
	EnterNode(StartNode);
	return true;
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
		|| (Mode != EConversationMode::Conversation && Mode != EConversationMode::Practice))
	{
		return FGradeResult();
	}

	const FGradeResult Result = USentenceGrader::Grade(*Challenge, Tokens);
	bLastAnswerPassed = Result.bPassed;
	bAwaitingContinueAfterGrade = true;

	if (ULanguageProgressSubsystem* Progress = GetGameInstanceSubsystem<ULanguageProgressSubsystem>(GetWorld()))
	{
		Progress->RecordResult(GetCityId(), Challenge->SkillTag, Result.bPassed);
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
			FailedItems.AddUnique({ CurrentNodeId, ActiveChoice });
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
