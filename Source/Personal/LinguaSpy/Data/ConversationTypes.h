// LinguaSpy - conversation data schema

#pragma once

#include "CoreMinimal.h"
#include "ConversationTypes.generated.h"

/** A single word (or word chunk, for languages without spaces) offered as a tile. */
USTRUCT(BlueprintType)
struct FWordTile
{
	GENERATED_BODY()

	/** The tile text in the target language (e.g. "café", "コーヒー"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tile")
	FString Text;

	/** Optional reading / romanization (e.g. "koohii"). Shown when the romaji toggle is on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tile")
	FString Reading;
};

/** One accepted word order for a challenge (sequence of tile texts). */
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge", meta=(TitleProperty="Text"))
	TArray<FWordTile> Tiles;

	/** Every word order that counts as correct. The first entry is shown as the model answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TArray<FAcceptedAnswer> AcceptedAnswers;

	/** Skill this challenge trains (e.g. Greetings, Ordering, Directions). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	FName SkillTag;

	/** Cover lost on a wrong answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge", meta=(ClampMin="0"))
	float CoverPenalty = 25.f;

	/** Finds the reading for a tile text, if any. */
	FString FindReading(const FString& TileText) const
	{
		const FWordTile* Tile = Tiles.FindByPredicate([&TileText](const FWordTile& T) { return T.Text == TileText; });
		return Tile ? Tile->Reading : FString();
	}
};

/** One option at a PlayerChoice node: what the player decides to say, and where it leads. */
USTRUCT(BlueprintType)
struct FConversationChoice
{
	GENERATED_BODY()

	/** Short label in the player's native language (e.g. "Play it cool - talk about the weather"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	FWordTileChallenge Challenge;

	/** Node to go to after a correct answer. None ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	FName NextNode;

	/** Node after a wrong answer. None returns to the choice so the player can try another approach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	FName FailNode;

	/** Mission flags set when this choice is answered correctly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	TArray<FName> SetFlags;

	/** Only offered when all of these mission flags are set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Choice")
	TArray<FName> RequiredFlags;
};

UENUM(BlueprintType)
enum class EConversationNodeType : uint8
{
	/** NPC says something; player clicks to continue. */
	NpcLine,
	/** NPC says something and the player must answer with tiles. */
	PlayerChallenge,
	/** NPC says something and the player picks what to say (each option is its own tile challenge). */
	PlayerChoice,
	/** NPC says a final line and the conversation ends with Outcome. */
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

	/** Optional reading / romanization of NativeLine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(MultiLine=true))
	FText NativeLineReading;

	/** The helper's translation of NativeLine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(MultiLine=true))
	FText HelperTranslation;

	/** Only used when Type == PlayerChallenge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChallenge", EditConditionHides))
	FWordTileChallenge Challenge;

	/** Only used when Type == PlayerChoice. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChoice", EditConditionHides, TitleProperty="Label"))
	TArray<FConversationChoice> Choices;

	/** Node to go to after this one (or after a correct answer). None ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	FName NextNode;

	/** Optional node to go to after a wrong answer. None means the player retries the same challenge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChallenge", EditConditionHides))
	FName FailNode;

	/** Mission flags set when this node is entered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	TArray<FName> SetFlags;

	/** Reported to the mission system when an End node finishes (None reports "Completed"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::End", EditConditionHides))
	FName Outcome;

	bool IsChallenge() const { return Type == EConversationNodeType::PlayerChallenge || Type == EConversationNodeType::PlayerChoice; }
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

namespace LinguaSpyOutcomes
{
	/** Reported when an End node has no explicit outcome. */
	inline const FName Completed(TEXT("Completed"));
}
