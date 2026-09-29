// LinguaSpy - authored conversation

#include "ConversationAsset.h"
#include "SentenceGrader.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "LinguaSpyConversationAsset"

const FConversationNode* UConversationAsset::FindNode(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}
	return Nodes.FindByPredicate([NodeId](const FConversationNode& Node) { return Node.NodeId == NodeId; });
}

const FWordTileChallenge* UConversationAsset::FindChallenge(FName NodeId, int32 ChoiceIndex) const
{
	const FConversationNode* Node = FindNode(NodeId);
	if (!Node)
	{
		return nullptr;
	}
	if (Node->Type == EConversationNodeType::PlayerChallenge)
	{
		return &Node->Challenge;
	}
	if (Node->Type == EConversationNodeType::PlayerChoice && Node->Choices.IsValidIndex(ChoiceIndex))
	{
		return &Node->Choices[ChoiceIndex].Challenge;
	}
	return nullptr;
}

namespace
{
	void ValidateChallenge(const FWordTileChallenge& Challenge, const FString& Where, TArray<FText>& Out)
	{
		if (Challenge.AcceptedAnswers.IsEmpty())
		{
			Out.Add(FText::Format(LOCTEXT("NoAnswers", "{0}: challenge has no accepted answers"), FText::FromString(Where)));
		}
		TArray<FString> TileTexts;
		for (const FWordTile& Tile : Challenge.Tiles)
		{
			TileTexts.Add(USentenceGrader::NormalizeToken(Tile.Text));
		}
		for (int32 i = 0; i < Challenge.AcceptedAnswers.Num(); ++i)
		{
			TArray<FString> Remaining = TileTexts;
			for (const FString& Token : Challenge.AcceptedAnswers[i].Tokens)
			{
				if (Remaining.RemoveSingle(USentenceGrader::NormalizeToken(Token)) == 0)
				{
					Out.Add(FText::Format(LOCTEXT("MissingTile", "{0}: answer {1} uses \"{2}\" which is not (or not often enough) in the tiles"),
						FText::FromString(Where), i + 1, FText::FromString(Token)));
				}
			}
		}
		if (Challenge.SkillTag.IsNone())
		{
			Out.Add(FText::Format(LOCTEXT("NoSkill", "{0}: challenge has no SkillTag"), FText::FromString(Where)));
		}
	}
}

TArray<FText> UConversationAsset::Validate() const
{
	TArray<FText> Out;
	TSet<FName> Ids;
	for (const FConversationNode& Node : Nodes)
	{
		if (Node.NodeId.IsNone())
		{
			Out.Add(LOCTEXT("EmptyId", "A node has no NodeId"));
		}
		else if (Ids.Contains(Node.NodeId))
		{
			Out.Add(FText::Format(LOCTEXT("DupId", "Duplicate node id '{0}'"), FText::FromName(Node.NodeId)));
		}
		Ids.Add(Node.NodeId);
	}

	if (!FindNode(StartNode))
	{
		Out.Add(FText::Format(LOCTEXT("BadStart", "StartNode '{0}' does not exist"), FText::FromName(StartNode)));
	}
	if (!Language)
	{
		Out.Add(LOCTEXT("NoLanguage", "No Language pack assigned"));
	}

	auto CheckLink = [this, &Out](FName From, FName To, const TCHAR* Kind)
	{
		if (!To.IsNone() && !FindNode(To))
		{
			Out.Add(FText::Format(LOCTEXT("Dangling", "Node '{0}': {1} '{2}' does not exist"), FText::FromName(From), FText::FromString(Kind), FText::FromName(To)));
		}
	};

	TSet<FName> Reachable;
	for (const FConversationNode& Node : Nodes)
	{
		const FString Where = Node.NodeId.ToString();
		if (Node.NativeLine.IsEmpty())
		{
			Out.Add(FText::Format(LOCTEXT("NoLine", "Node '{0}' has no NativeLine"), FText::FromName(Node.NodeId)));
		}
		if (Node.HelperTranslation.IsEmpty())
		{
			Out.Add(FText::Format(LOCTEXT("NoTranslation", "Node '{0}' has no HelperTranslation"), FText::FromName(Node.NodeId)));
		}

		CheckLink(Node.NodeId, Node.NextNode, TEXT("NextNode"));
		Reachable.Add(Node.NextNode);
		switch (Node.Type)
		{
		case EConversationNodeType::PlayerChallenge:
			ValidateChallenge(Node.Challenge, Where, Out);
			CheckLink(Node.NodeId, Node.FailNode, TEXT("FailNode"));
			Reachable.Add(Node.FailNode);
			break;
		case EConversationNodeType::PlayerChoice:
			if (Node.Choices.IsEmpty())
			{
				Out.Add(FText::Format(LOCTEXT("NoChoices", "Choice node '{0}' has no choices"), FText::FromName(Node.NodeId)));
			}
			for (int32 i = 0; i < Node.Choices.Num(); ++i)
			{
				const FConversationChoice& Choice = Node.Choices[i];
				ValidateChallenge(Choice.Challenge, FString::Printf(TEXT("%s choice %d"), *Where, i + 1), Out);
				CheckLink(Node.NodeId, Choice.NextNode, TEXT("choice NextNode"));
				CheckLink(Node.NodeId, Choice.FailNode, TEXT("choice FailNode"));
				Reachable.Add(Choice.NextNode);
				Reachable.Add(Choice.FailNode);
			}
			break;
		default:
			break;
		}
	}

	for (const FConversationNode& Node : Nodes)
	{
		if (Node.NodeId != StartNode && !Reachable.Contains(Node.NodeId))
		{
			Out.Add(FText::Format(LOCTEXT("Unreachable", "Node '{0}' is unreachable"), FText::FromName(Node.NodeId)));
		}
	}
	return Out;
}

#if WITH_EDITOR
EDataValidationResult UConversationAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (const FText& Problem : Validate())
	{
		Context.AddWarning(Problem);
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
