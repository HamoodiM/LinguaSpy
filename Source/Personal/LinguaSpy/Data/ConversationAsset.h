// LinguaSpy - authored conversation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConversationTypes.h"
#include "ConversationAsset.generated.h"

class ULanguagePackAsset;

/** An authored conversation between the spy and one NPC. */
UCLASS(BlueprintType)
class UConversationAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	FText Title;

	/** Language/city this conversation belongs to (controls word spacing, readings, skill keys). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	TObjectPtr<ULanguagePackAsset> Language;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	FName StartNode;

	/**
	 *  If set, blowing your cover in this conversation ends it with this outcome (after the practice drill),
	 *  letting the mission branch. If None, the conversation simply restarts after the drill.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	FName CoverBlownOutcome;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation", meta=(TitleProperty="NodeId"))
	TArray<FConversationNode> Nodes;

	/** Returns the node with the given id, or nullptr. */
	const FConversationNode* FindNode(FName NodeId) const;

	/** Returns the challenge for a node (ChoiceIndex is used for PlayerChoice nodes), or nullptr. */
	const FWordTileChallenge* FindChallenge(FName NodeId, int32 ChoiceIndex) const;

	/** Validation problems (dangling links, duplicate ids, answer words missing from tiles...). Empty if valid. */
	TArray<FText> Validate() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
