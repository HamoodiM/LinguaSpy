// LinguaSpy - runs the active conversation and cover-blown practice drills

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConversationTypes.h"
#include "ConversationSubsystem.generated.h"

class ANpcCharacter;
class UConversationAsset;
class UCoverComponent;
class ULanguagePackAsset;

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
	Practice,
	/** Spaced-repetition review from the Safe house (no NPC, no cover, no mission). */
	Review
};

/**
 *  A challenge to replay in a practice drill or review (ChoiceIndex is set for PlayerChoice nodes).
 *  Conversation is kept alive by ActiveConversation / ReviewConversations.
 */
struct FConversationPracticeItem
{
	UConversationAsset* Conversation = nullptr;
	FName NodeId;
	int32 ChoiceIndex = INDEX_NONE;
	bool operator==(const FConversationPracticeItem& Other) const
	{
		return Conversation == Other.Conversation && NodeId == Other.NodeId && ChoiceIndex == Other.ChoiceIndex;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationStarted, ANpcCharacter*, Npc);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationNodeEntered, const FConversationNode&, Node, EConversationMode, Mode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChallengeReady, const FWordTileChallenge&, Challenge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnswerGraded, const FGradeResult&, Result, const FString&, ModelAnswer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationCoverBlown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReviewFinished, int32, Passed, int32, Total);

struct FSrsItemKey;

/**
 *  Drives one conversation at a time:
 *  NPC lines -> word-tile challenges / choices -> grading -> cover penalties -> outcome reported to missions.
 *  When cover is blown, the failed challenges are replayed as a practice drill; afterwards the conversation
 *  restarts, or ends with the asset's CoverBlownOutcome so the mission can branch.
 */
UCLASS()
class PERSONAL_API UConversationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationStarted OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationEnded OnConversationEnded;

	/** A new node is shown. For PlayerChoice nodes the UI shows the available choices. */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationNodeEntered OnNodeEntered;

	/** A tile challenge is ready to answer (right after a PlayerChallenge node, or after a choice is picked). */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnChallengeReady OnChallengeReady;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnAnswerGraded OnAnswerGraded;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationCoverBlown OnCoverBlown;

	/** A review session ran out of items (first-attempt passes / items). Broadcast right before OnConversationEnded. */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnReviewFinished OnReviewFinished;

	/** Starts talking to the NPC. Returns false if busy or the NPC has no conversation. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartConversation(ANpcCharacter* Npc, UCoverComponent* PlayerCover);

	/**
	 *  Starts a conversation directly from an asset (no NPC), optionally at a specific node.
	 *  Used by the editor's "Play from node" and for debugging. No mission outcome is reported.
	 */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartConversationFromAsset(UConversationAsset* Conversation, FName StartNodeOverride, UCoverComponent* PlayerCover);

	/** Starts a spaced-repetition review of the given challenges (Safe house). Returns false if busy or nothing resolves. */
	bool StartReviewSession(const TArray<FSrsItemKey>& Items);

	/** Python/Blueprint wrapper: reviews up to Max items for a city (due first, then weakest). */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartReviewForCity(FName CityId, int32 Max = 10);

	/**
	 *  The player taps "Translate" on the current line. The first reveal per node counts as a peek
	 *  (lowers the fluency grade; answers on this node count as assisted for spaced repetition).
	 */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	void RevealTranslation();

	/** Whether the helper's translation of the current line is revealed (tapped, or "always show" is on). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsTranslationRevealed() const;

	/** Tallies for the running conversation (graded answers, wrong answers, peeks). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	void GetFluencyTally(int32& OutChallenges, int32& OutMistakes, int32& OutPeeks) const;

	/** Picks an option at a PlayerChoice node. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool SelectChoice(int32 ChoiceIndex);

	/** Whether a choice at the current node is offered (its RequiredFlags are met). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsChoiceAvailable(int32 ChoiceIndex) const;

	/** Grades the player's tiles for the current challenge. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	FGradeResult SubmitAnswer(const TArray<FString>& Tokens);

	/** Moves on from the current line / graded answer / cover-blown screen. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	void Continue();

	/** Leaves the conversation immediately (no outcome is reported). */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	void EndConversation();

	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsActive() const { return Mode != EConversationMode::None; }

	UFUNCTION(BlueprintPure, Category="Conversation")
	EConversationMode GetMode() const { return Mode; }

	UFUNCTION(BlueprintPure, Category="Conversation")
	ANpcCharacter* GetActiveNpc() const { return ActiveNpc.Get(); }

	UFUNCTION(BlueprintPure, Category="Conversation")
	ULanguagePackAsset* GetActiveLanguage() const;

	/** Number of challenges left in the current practice drill. */
	UFUNCTION(BlueprintPure, Category="Conversation")
	int32 GetPracticeRemaining() const { return PracticeQueue.Num(); }

	/** Items in the running review session (for "3 / 10" counters). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	int32 GetReviewTotal() const { return ReviewTotal; }

	/** Cover restored for each correct answer during a conversation. */
	UPROPERTY(EditAnywhere, Category="Conversation")
	float CoverRecoveryOnPass = 5.f;

	virtual void Deinitialize() override;

private:

	bool BeginConversation(ANpcCharacter* Npc, UConversationAsset* Conversation, FName StartNode, UCoverComponent* PlayerCover);
	void EnterNode(FName NodeId);
	void EnterPracticeItem();
	void FinishReview();
	bool IsAssistedAnswer() const;
	void RecordSrs(const FWordTileChallenge& Challenge, bool bPassed);
	void FinishConversation(FName Outcome);
	const FConversationNode* GetCurrentNode() const;
	const FWordTileChallenge* GetActiveChallenge() const;
	void ApplyFlags(const TArray<FName>& Flags) const;
	FName GetCityId() const;

	UFUNCTION()
	void HandleCoverBlown();

	UPROPERTY()
	TObjectPtr<UConversationAsset> ActiveConversation;

	TWeakObjectPtr<ANpcCharacter> ActiveNpc;
	TWeakObjectPtr<UCoverComponent> ActiveCover;

	EConversationMode Mode = EConversationMode::None;
	FName CurrentNodeId;

	/** Chosen option at a PlayerChoice node (INDEX_NONE until picked). */
	int32 ActiveChoice = INDEX_NONE;

	/** Whether the current challenge has been graded, and how it went. */
	bool bAwaitingContinueAfterGrade = false;
	bool bLastAnswerPassed = false;

	/** Challenges failed during this attempt, replayed if the cover gets blown. */
	TArray<FConversationPracticeItem> FailedItems;

	/** Remaining practice challenges. */
	TArray<FConversationPracticeItem> PracticeQueue;

	/** Conversations used by the running review (keeps them loaded). */
	UPROPERTY()
	TArray<TObjectPtr<UConversationAsset>> ReviewConversations;

	/** Review items already graded once this session (only first attempts feed spaced repetition). */
	TArray<FConversationPracticeItem> ReviewAttempted;
	int32 ReviewTotal = 0;
	int32 ReviewPassed = 0;

	/** Nodes whose translation the player revealed in this conversation. */
	TSet<FName> RevealedNodes;
	int32 TallyChallenges = 0;
	int32 TallyMistakes = 0;
	int32 TallyPeeks = 0;
};
