// LinguaSpy - authored conversation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConversationTypes.h"
#include "ConversationAsset.generated.h"

/** An authored conversation between the spy and one NPC. */
UCLASS(BlueprintType)
class UConversationAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	FName StartNode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation", meta=(TitleProperty="NodeId"))
	TArray<FConversationNode> Nodes;

	/** Returns the node with the given id, or nullptr. */
	const FConversationNode* FindNode(FName NodeId) const;
};
