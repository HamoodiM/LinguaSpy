// LinguaSpy - runs the active conversation and cover-blown practice drills

#include "ConversationSubsystem.h"
#include "ConversationAsset.h"
#include "CoverComponent.h"
#include "NpcCharacter.h"
#include "SentenceGrader.h"
#include "LanguageProgressSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Personal.h"

bool UConversationSubsystem::StartConversation(ANpcCharacter* Npc, UCoverComponent* PlayerCover)
{
	if (IsActive() || !Npc || !Npc->GetConversation())
	{
		return false;
	}

	ActiveNpc = Npc;
	ActiveConversation = Npc->GetConversation();
	ActiveCover = PlayerCover;
	FailedNodeIds.Reset();
	PracticeQueue.Reset();
	Mode = EConversationMode::Conversation;

	if (PlayerCover)
	{
		PlayerCover->OnCoverBlown.AddUniqueDynamic(this, &UConversationSubsystem::HandleCoverBlown);
	}

	OnConversationStarted.Broadcast(Npc);
	EnterNode(ActiveConversation->StartNode);
	return true;
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
		EndConversation();
		return;
	}

	CurrentNodeId = NodeId;
	bAwaitingContinueAfterGrade = false;
	bLastAnswerPassed = false;
	OnNodeEntered.Broadcast(*Node, Mode);
}

void UConversationSubsystem::EnterPracticeItem()
{
	if (PracticeQueue.IsEmpty())
	{
		// Drill complete: restore cover and try the real conversation again
		if (UCoverComponent* Cover = ActiveCover.Get())
		{
			Cover->ResetCover();
		}
		FailedNodeIds.Reset();
		Mode = EConversationMode::Conversation;
		EnterNode(ActiveConversation->StartNode);
		return;
	}
	EnterNode(PracticeQueue[0]);
}

const FConversationNode* UConversationSubsystem::GetCurrentNode() const
{
	return ActiveConversation ? ActiveConversation->FindNode(CurrentNodeId) : nullptr;
}

FGradeResult UConversationSubsystem::SubmitAnswer(const TArray<FString>& Tokens)
{
	const FConversationNode* Node = GetCurrentNode();
	if (!Node || Node->Type != EConversationNodeType::PlayerChallenge || bAwaitingContinueAfterGrade
		|| (Mode != EConversationMode::Conversation && Mode != EConversationMode::Practice))
	{
		return FGradeResult();
	}

	const FGradeResult Result = USentenceGrader::Grade(Node->Challenge, Tokens);
	bLastAnswerPassed = Result.bPassed;
	bAwaitingContinueAfterGrade = true;

	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (ULanguageProgressSubsystem* Progress = GameInstance->GetSubsystem<ULanguageProgressSubsystem>())
		{
			Progress->RecordResult(Node->Challenge.SkillTag, Result.bPassed);
		}
	}

	FString ModelAnswer;
	if (Node->Challenge.AcceptedAnswers.Num() > 0)
	{
		ModelAnswer = USentenceGrader::JoinTokens(Node->Challenge.AcceptedAnswers[0].Tokens);
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
			FailedNodeIds.AddUnique(CurrentNodeId);
			if (Cover)
			{
				Cover->ApplyPenalty(Node->Challenge.CoverPenalty); // may trigger HandleCoverBlown
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
		PracticeQueue = FailedNodeIds;
		Mode = EConversationMode::Practice;
		EnterPracticeItem();
		return;

	case EConversationMode::Practice:
		if (bAwaitingContinueAfterGrade && PracticeQueue.Num() > 0)
		{
			const FName Item = PracticeQueue[0];
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
			EndConversation();
			break;
		case EConversationNodeType::PlayerChallenge:
			if (!bAwaitingContinueAfterGrade)
			{
				break; // must answer first
			}
			if (bLastAnswerPassed)
			{
				EnterNode(Node->NextNode);
			}
			else
			{
				EnterNode(Node->FailNode.IsNone() ? CurrentNodeId : Node->FailNode);
			}
			break;
		}
		return;
	}
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
	PracticeQueue.Reset();
	FailedNodeIds.Reset();
	OnConversationEnded.Broadcast();
}

void UConversationSubsystem::Deinitialize()
{
	EndConversation();
	Super::Deinitialize();
}
