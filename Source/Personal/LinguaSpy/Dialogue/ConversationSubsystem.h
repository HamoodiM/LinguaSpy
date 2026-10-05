// LinguaSpy - runs the active conversation, cover-blown practice drills, Safe house reviews and mission debriefs

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConversationTypes.h"
#include "SrsScheduler.h"
#include "LanguagePackAsset.h"
#include "ConversationSubsystem.generated.h"

class ANpcCharacter;
class UConversationAsset;
class UCoverComponent;
class UMissionAsset;

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
	/** Spaced-repetition review from the Safe house, or a mission debrief (no NPC, no cover, no mission). */
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

/** How a challenge is presented this time (scaffold level and the material for it). */
USTRUCT(BlueprintType)
struct PERSONAL_API FChallengePresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	FWordTileChallenge Challenge;

	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	EScaffoldLevel Level = EScaffoldLevel::AssemblePlus;

	/** Tiles to offer (Assemble / AssemblePlus). */
	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	TArray<FWordTile> Tiles;

	/** Target-language sentences to pick from (Recognize). */
	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	TArray<FString> RecognitionOptions;

	/** The model sentence with blanks (GapFill), e.g. "Bonjour, je ____ un café". */
	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	FString GapDisplay;

	/** Number of words to type (GapFill). */
	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	int32 GapCount = 0;

	/** 1 on the first try, 2 after a prompt. */
	UPROPERTY(BlueprintReadOnly, Category="Challenge")
	int32 Attempt = 1;
};

/** Result of one submission, with the feedback-ladder step it triggered. */
USTRUCT(BlueprintType)
struct PERSONAL_API FAnswerFeedback
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Feedback")
	FGradeResult Grade;

	/** False after a first mistake: the player gets a hint (Hint) and tries again. True = Continue moves on. */
	UPROPERTY(BlueprintReadOnly, Category="Feedback")
	bool bFinal = true;

	/** Prompt shown after a first mistake (never the answer itself). */
	UPROPERTY(BlueprintReadOnly, Category="Feedback")
	FText Hint;

	/** The model answer (only filled on a final result). */
	UPROPERTY(BlueprintReadOnly, Category="Feedback")
	FString ModelAnswer;

	/** Passed with support (recognition level, second attempt, or after peeking): doesn't promote the SRS item. */
	UPROPERTY(BlueprintReadOnly, Category="Feedback")
	bool bAssisted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationStarted, ANpcCharacter*, Npc);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationNodeEntered, const FConversationNode&, Node, EConversationMode, Mode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChallengeReady, const FChallengePresentation&, Presentation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnswerFeedback, const FAnswerFeedback&, Feedback);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversationCoverBlown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReviewFinished, int32, Passed, int32, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIntelAnswered, bool, bCorrect, int32, ChoiceIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFieldNoteUnlocked, const FGrammarNote&, Note);

/**
 *  Drives one conversation at a time:
 *  NPC lines (with voice) -> scaffolded challenges / choices / intel checks -> feedback ladder -> cover -> mission outcome.
 *  Scaffold level per challenge comes from its SRS strength (UScaffoldPolicy). A first mistake gets a hint (prompt), a
 *  second one the model answer (recast). When cover is blown, the failed challenges are replayed as a practice drill;
 *  afterwards the conversation restarts, or ends with the asset's CoverBlownOutcome so the mission can branch.
 *  See docs/Learning_Design.md for the research behind each step.
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

	/** A new node is shown. For PlayerChoice nodes the UI shows the available choices; IntelCheck nodes show Intel. */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationNodeEntered OnNodeEntered;

	/** A challenge is ready to answer (right after a PlayerChallenge node, after a choice is picked, or for a retry). */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnChallengeReady OnChallengeReady;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnAnswerFeedback OnAnswerFeedback;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnConversationCoverBlown OnCoverBlown;

	/** A review session ran out of items (first-attempt passes / items). Broadcast right before OnConversationEnded. */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnReviewFinished OnReviewFinished;

	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnIntelAnswered OnIntelAnswered;

	/** A field note reached its encounter threshold. Broadcast after the conversation ends (not mid-dialogue). */
	UPROPERTY(BlueprintAssignable, Category="Conversation")
	FOnFieldNoteUnlocked OnFieldNoteUnlocked;

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
	bool StartReviewSession(const TArray<FSrsItemKey>& Items, bool bDebrief = false);

	/** Python/Blueprint wrapper: reviews up to Max items for a city (due first, then weakest). */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartReviewForCity(FName CityId, int32 Max = 10);

	/** Post-mission retrieval practice (no cover). Returns false if busy or nothing resolves. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool StartDebrief(const TArray<FSrsItemKey>& Items);

	/** True while the running review is a mission debrief. */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsDebrief() const { return Mode == EConversationMode::Review && bDebrief; }

	/**
	 *  The player taps "Translate" on the current line. The first reveal per node counts as a peek (lowers the fluency
	 *  grade; answers on this node count as assisted). Returns false if refused (Radio silence: no peeks left).
	 */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool RevealTranslation();

	/** Whether the helper's translation of the current line is visible (tapped, or the handler is in Shadowing mode). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsTranslationRevealed() const;

	/** Peeks left in this conversation (-1 = unlimited). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	int32 GetPeeksLeft() const;

	/** Tallies for the running conversation (graded answers, wrong answers, peeks). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	void GetFluencyTally(int32& OutChallenges, int32& OutMistakes, int32& OutPeeks) const;

	/** Picks an option at a PlayerChoice node. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool SelectChoice(int32 ChoiceIndex);

	/** Whether a choice at the current node is offered (its RequiredFlags are met). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool IsChoiceAvailable(int32 ChoiceIndex) const;

	/** Grades tiles (Assemble / AssemblePlus) or typed words (Produce: TokenizeTyped first) for the current challenge. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	FAnswerFeedback SubmitAnswer(const TArray<FString>& Tokens);

	/** GapFill: the typed words for the blanks, in order. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	FAnswerFeedback SubmitGapWords(const TArray<FString>& Words);

	/** Recognize: index into FChallengePresentation::RecognitionOptions. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	FAnswerFeedback SubmitRecognition(int32 OptionIndex);

	/** IntelCheck node: picks an option. Correct -> Continue moves on; wrong -> suspicion, try again. */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool SubmitIntel(int32 OptionIndex);

	/** Current presentation (valid while a challenge is open). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	FChallengePresentation GetPresentation() const { return Presentation; }

	/** Plays the current line again (the first replay per line is free, later ones cost a little cover). */
	UFUNCTION(BlueprintCallable, Category="Conversation")
	bool RepeatLine(bool bSlow);

	/** Whether the current line has a recording (Repeat / Slow buttons). */
	UFUNCTION(BlueprintPure, Category="Conversation")
	bool HasLineAudio() const;

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

	/** Cover lost per replay of a line after the first free one. */
	UPROPERTY(EditAnywhere, Category="Conversation")
	float RepeatCost = 2.f;

	/** Peeks per conversation in Radio silence mode. */
	static constexpr int32 RadioSilencePeeks = 2;

	virtual void Deinitialize() override;

private:

	bool BeginConversation(ANpcCharacter* Npc, UConversationAsset* Conversation, FName StartNode, UCoverComponent* PlayerCover);
	void EnterNode(FName NodeId);
	void EnterPracticeItem();
	void FinishReview();
	void PresentChallenge(const FWordTileChallenge& Challenge);
	FAnswerFeedback ApplyGrade(const FGradeResult& Result, int32 SubmittedTokens);
	void RecordFinal(const FWordTileChallenge& Challenge, bool bPassed, bool bAssisted, bool bAccentOnly);
	void CountFieldNotes(const FWordTileChallenge& Challenge);
	void PlayLine(const FConversationNode& Node, bool bSlow);
	bool CanAnswer() const;
	bool IsPeeked() const;
	FSrsItemKey GetCurrentKey() const;
	void FinishConversation(FName Outcome);
	const FConversationNode* GetCurrentNode() const;
	const FWordTileChallenge* GetActiveChallenge() const;
	void ApplyFlags(const TArray<FName>& Flags) const;
	FName GetCityId() const;
	FString ModelAnswerFor(const FWordTileChallenge& Challenge) const;

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

	/** Whether the current challenge has been graded (final), and how it went. */
	bool bAwaitingContinueAfterGrade = false;
	bool bLastAnswerPassed = false;

	/** The open challenge: scaffold, attempt, recognition answer, timing. */
	FChallengePresentation Presentation;
	int32 RecognitionCorrect = INDEX_NONE;
	FDateTime ChallengeStart;
	int32 IntelAttempts = 0;

	/** Wrong attempts per SRS key in this session (scaffold steps down). */
	TMap<FString, int32> SessionErrors;

	/** Replays of the current line (first is free). */
	int32 LineReplays = 0;

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
	bool bDebrief = false;

	/** Nodes whose translation the player revealed in this conversation. */
	TSet<FName> RevealedNodes;
	int32 TallyChallenges = 0;
	int32 TallyMistakes = 0;
	int32 TallyPeeks = 0;

	/** Field notes unlocked during this conversation (announced when it ends). */
	TArray<FName> PendingFieldNotes;
};
