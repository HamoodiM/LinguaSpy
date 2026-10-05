// LinguaSpy - conversation data schema

#pragma once

#include "CoreMinimal.h"
#include "ConversationTypes.generated.h"

class USoundBase;

/** A single word (or word chunk, for languages without spaces) offered as a tile. */
USTRUCT(BlueprintType)
struct PERSONAL_API FWordTile
{
	GENERATED_BODY()

	/** The tile text in the target language (e.g. "café", "コーヒー"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tile")
	FString Text;

	/** Optional reading / romanization (e.g. "koohii"). Shown when the romaji toggle is on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tile")
	FString Reading;
};

/**
 *  Authored hint for the feedback ladder: shown after a first wrong attempt instead of the model answer
 *  (a prompt makes the learner self-correct; see docs/Learning_Design.md section 3).
 */
USTRUCT(BlueprintType)
struct PERSONAL_API FChallengeHint
{
	GENERATED_BODY()

	/** Token index (in the first accepted answer) this hint is about; -1 = general hint for the whole answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hint")
	int32 TokenIndex = -1;

	/** E.g. "Politeness: strangers get vous, not tu." (player's native language). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hint", meta=(MultiLine=true))
	FText Text;
};

/** One accepted word order for a challenge (sequence of tile texts). */
USTRUCT(BlueprintType)
struct PERSONAL_API FAcceptedAnswer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Answer")
	TArray<FString> Tokens;
};

/** A reply the player must build from word tiles. */
USTRUCT(BlueprintType)
struct PERSONAL_API FWordTileChallenge
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

	/** Cover lost on a wrong answer (split across the feedback ladder: a share on the prompt, the rest on the recast). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge", meta=(ClampMin="0"))
	float CoverPenalty = 25.f;

	/** Feedback-ladder hints. The hint for the first wrong token wins, then a general one (-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge", meta=(TitleProperty="Text"))
	TArray<FChallengeHint> Hints;

	/** Gap-fill scaffold: token indices (in the first accepted answer) the player types. Empty = picked automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TArray<int32> GapTokenIndices;

	/** Recognition scaffold: wrong full sentences in the target language. Empty = generated from distractor tiles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TArray<FString> RecognitionDistractors;

	/** Spoken model answer (played on the recast and in the briefing). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Challenge")
	TSoftObjectPtr<USoundBase> ModelAudio;

	/** Finds the reading for a tile text, if any. */
	FString FindReading(const FString& TileText) const
	{
		const FWordTile* Tile = Tiles.FindByPredicate([&TileText](const FWordTile& T) { return T.Text == TileText; });
		return Tile ? Tile->Reading : FString();
	}
};

/** One option at a PlayerChoice node: what the player decides to say, and where it leads. */
USTRUCT(BlueprintType)
struct PERSONAL_API FConversationChoice
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
	End,
	/** NPC gives information the player must act on (listening comprehension); the player picks the right option. */
	IntelCheck
};

/** A listening-comprehension check: "The palace opens at ten" -> "When does it open?" [9 / 10 / 2]. */
USTRUCT(BlueprintType)
struct PERSONAL_API FIntelCheck
{
	GENERATED_BODY()

	/** Question in the player's native language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel")
	FText Question;

	/** Answer options in the player's native language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel")
	TArray<FText> Options;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel")
	int32 CorrectIndex = 0;

	/** Mission flag set when answered correctly (missions can require it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel")
	FName IntelFlag;

	/** Cover lost per wrong pick (asking again looks suspicious). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel", meta=(ClampMin="0"))
	float SuspicionCost = 10.f;

	/** Skill this check trains (for stats/telemetry), e.g. Time, Numbers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intel")
	FName SkillTag;
};

/** A single step in a conversation graph. */
USTRUCT(BlueprintType)
struct PERSONAL_API FConversationNode
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

	/** Spoken NativeLine (generated offline by Tools/Audio/generate_voice.py). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	TSoftObjectPtr<USoundBase> LineAudio;

	/** Slowed-down NativeLine for the "Slow" button. Falls back to LineAudio at a lower pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node")
	TSoftObjectPtr<USoundBase> LineAudioSlow;

	/** Only used when Type == PlayerChallenge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChallenge", EditConditionHides))
	FWordTileChallenge Challenge;

	/** Only used when Type == PlayerChoice. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::PlayerChoice", EditConditionHides, TitleProperty="Label"))
	TArray<FConversationChoice> Choices;

	/** Only used when Type == IntelCheck. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Node", meta=(EditCondition="Type==EConversationNodeType::IntelCheck", EditConditionHides))
	FIntelCheck Intel;

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

	/** Failed, but would pass if accents were ignored ("cafe" for "café"): worth a targeted hint. */
	UPROPERTY(BlueprintReadOnly, Category="Grade")
	bool bAccentOnly = false;

	/** Index of the first wrong token (or the first missing position), INDEX_NONE if passed. */
	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 FirstWrongIndex = INDEX_NONE;
};

/**
 *  Scaffold ladder (docs/Learning_Design.md section 2): how much support a challenge gets, from SRS strength.
 *  Values are ordered from most to least support.
 */
UENUM(BlueprintType)
enum class EScaffoldLevel : uint8
{
	/** Pick which sentence matches the intent (recognition). */
	Recognize,
	/** Tiles without distractors (ordering only). */
	Assemble,
	/** Tiles with distractors (the classic challenge). */
	AssemblePlus,
	/** Model sentence with 1-2 key words blanked; type them. */
	GapFill,
	/** Type the whole sentence. */
	Produce
};

namespace LinguaSpyOutcomes
{
	/** Reported when an End node has no explicit outcome. */
	inline const FName Completed(TEXT("Completed"));
}
