// LinguaSpy - runs the active conversation and cover-blown practice drills

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConversationTypes.h"
#include "ConversationSubsystem.generated.h"

class ANpcCharacter;
class UConversationAsset;
class UCoverComponent;

UENUM(BlueprintType)
enum class EConversationMode : uint8
{
	/** No conversation running. */
	None,
	/** Talking to an NPC, mistakes cost cover. */
	Conversation,
	/** Cover was blown; waiting for the player to acknowledge. */
	CoverBlown,
	/** Replaying failed challenges without cover pressure. */
	Practice
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationStarted, ANpcCharacter*, Npc);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationNodeEntered, const FConversationNode&, Node, EConversationMode, Mode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnswerGraded, const FGradeResult&, Result, const FString&, ModelAnswer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationCoverBlown);

/**
 *  Drives one conversation at a time:
 *  NPC lines -> word-tile challenges -> grading -> cover penalties.
 *  When cover is blown, the failed challenges are replayed as a practice drill,
 *  after which cover is restored and the conversation restarts.
 */
UCLASS()
class UConversationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationStarted OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationEnded OnConversationEnded;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationNodeEntered OnNodeEntered;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnAnswerGraded OnAnswerGraded;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationCoverBlown OnCoverBlown;

	/** Starts talking to the NPC. Returns false if busy or the NPC has no conversation. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartConversation(ANpcCharacter* Npc, UCoverComponent* PlayerCover);

	/** Grades the player's tiles for the current challenge. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	FGradeResult SubmitAnswer(const TArray<FString>& Tokens);

	/** Moves on from the current line / graded answer / cover-blown screen. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	void Continue();

	/** Leaves the conversation immediately. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	void EndConversation();

	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsActive() const { return Mode != EConversationMode::None; }

	UFUNCTION(BlueprintPure, Category="Conversation")
	EConversationMode GetMode() const { return Mode; }

	UFUNCTION(BlueprintPure, Category="Conversation")
	ANpcCharacter* GetActiveNpc() const { return ActiveNpc.Get(); }

	/** Number of challenges left in the current practice drill. */
	UFUNCTION(BlueprintPure, Category="Conversation")
	int32 GetPracticeRemaining() const { return PracticeQueue.Num(); }

	/** Cover restored for each correct answer during a conversation. */
	UPROPERTY(EditAnywhere, Category="Conversation")
	float CoverRecoveryOnPass = 5.f;

	virtual void Deinitialize() override;

private:

	void EnterNode(FName NodeId);
	void EnterPracticeItem();
	const FConversationNode* GetCurrentNode() const;

	UFUNCTION()
	void HandleCoverBlown();

	UPROPERTY()
	TObjectPtr<UConversationAsset> ActiveConversation;

	TWeakObjectPtr<ANpcCharacter> ActiveNpc;
	TWeakObjectPtr<UCoverComponent> ActiveCover;

	EConversationMode Mode = EConversationMode::None;
	FName CurrentNodeId;

	/** Whether the current challenge has been graded, and how it went. */
	bool bAwaitingContinueAfterGrade = false;
	bool bLastAnswerPassed = false;

	/** Challenges failed during this attempt, replayed if the cover gets blown. */
	TArray<FName> FailedNodeIds;

	/** Remaining practice challenges (node ids). */
	TArray<FName> PracticeQueue;
};
