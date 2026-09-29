// LinguaSpy - conversation data schema

#pragma once

#include "CoreMinimal.h"
#include "ConversationTypes.generated.h"

/** One accepted word order for a challenge (sequence of tile words). */
USTRUCT(BlueprintType)
struct FAcceptedAnswer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Answer")
	TArray<FString> Tokens;
};

/** A reply the player must build from word tiles. */
USTRUCT(BlueprintType)
struct FWordTileChallenge
{
	GENERATED_BODY()

	/** What the player should say, in their native language (e.g. "Politely order a coffee"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	FText IntentPrompt;

	/** All tiles offered: the correct words plus distractors. Shuffled at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TArray<FString> TileBank;

	/** Every word order that counts as correct. The first entry is shown as the model answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TArray<FAcceptedAnswer> AcceptedAnswers;

	/** Skill this challenge trains (e.g. Greetings, Ordering, Directions). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	FName SkillTag;

	/** Cover lost on a wrong answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge", meta=(ClampMin="0"))
	float CoverPenalty = 25.f;
};

UENUM(BlueprintType)
enum class EConversationNodeType : uint8
{
	/** NPC says something; player clicks to continue. */
	NpcLine,
	/** NPC says something and the player must answer with tiles. */
	PlayerChallenge,
	/** NPC says a final line and the conversation ends. */
	End
};

/** A single step in a conversation graph. */
USTRUCT(BlueprintType)
struct FConversationNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	EConversationNodeType Type = EConversationNodeType::NpcLine;

	/** What the NPC says, in the target language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(MultiLine=true))
	FText NativeLine;

	/** The helper's translation of NativeLine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(MultiLine=true))
	FText HelperTranslation;

	/** Only used when Type == PlayerChallenge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChallenge"))
	FWordTileChallenge Challenge;

	/** Node to go to after this one (or after a correct answer). None ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	FName NextNode;

	/** Optional node to go to after a wrong answer. None means the player retries the same challenge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChallenge"))
	FName FailNode;
};

/** Result of grading a tile answer. */
USTRUCT(BlueprintType)
struct FGradeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Grade")
	bool bPassed = false;

	/** Per submitted tile: true if it matches the closest accepted answer at that position. */
	UPROPERTY(BlueprintReadOnly, Category="Grade")
	TArray<bool> TileCorrect;

	/** Index into AcceptedAnswers of the closest answer (for showing the model answer). */
	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 ClosestAnswerIndex = INDEX_NONE;
};
