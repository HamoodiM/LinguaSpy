// LinguaSpy - scaffold ladder: how much support a challenge gets (docs/Learning_Design.md sections 2-3)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConversationTypes.h"
#include "ScaffoldPolicy.generated.h"

struct FSrsItemState;

/**
 *  Pure, testable scaffolding rules.
 *  Level from SRS strength: new / box 0 -> Assemble, box 1-2 -> AssemblePlus, box 3-4 -> GapFill, box 5 -> Produce.
 *  Each wrong attempt at the item in this session steps down one level (minimum Recognize).
 *  Typing levels need a Latin-script language; otherwise the level is capped at AssemblePlus.
 *  Feedback ladder: attempt 1 wrong -> prompt (hint, PromptPenaltyShare of the cover penalty), attempt 2 wrong -> recast
 *  (model answer, the rest of the penalty).
 */
UCLASS()
class PERSONAL_API UScaffoldPolicy : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Share of a challenge's cover penalty charged on the first (prompted) mistake. */
	static constexpr float PromptPenaltyShare = 0.4f;

	/** Attempts per challenge visit: attempt 1 wrong -> prompt, attempt 2 wrong -> recast. */
	static constexpr int32 MaxAttempts = 2;

	/** State may be null (never seen). SessionErrors = wrong attempts at this item earlier in this session. */
	static EScaffoldLevel ChooseLevel(const FSrsItemState* State, int32 SessionErrors, bool bTypingAllowed);

	/** Tiles to show: Assemble = only the first answer's tiles (no distractors); AssemblePlus = all tiles. */
	static TArray<FWordTile> BuildTiles(const FWordTileChallenge& Challenge, EScaffoldLevel Level);

	/** Token indices to blank for GapFill: authored, else the longest token (two for answers of 6+ tokens). */
	static TArray<int32> GetGapIndices(const FWordTileChallenge& Challenge);

	/** The first answer with gaps replaced by "____" (for display). */
	static FString BuildGapDisplay(const FWordTileChallenge& Challenge, bool bUseSpaces);

	/** First answer with the typed gap words put back in (for grading). */
	static TArray<FString> FillGaps(const FWordTileChallenge& Challenge, const TArray<FString>& TypedGapWords);

	/**
	 *  Recognition options (target language): the model answer plus up to 2 wrong sentences (authored, else made by
	 *  swapping a distractor tile into the answer, else by swapping two words). Shuffled with Seed.
	 */
	static void BuildRecognitionOptions(const FWordTileChallenge& Challenge, bool bUseSpaces, int32 Seed,
		TArray<FString>& OutOptions, int32& OutCorrectIndex);

	/** Passing with this much support doesn't promote the SRS item (recognition, or a second attempt after a prompt). */
	static bool IsAssistedPass(EScaffoldLevel Level, int32 Attempt);

	/**
	 *  Hint text after a wrong attempt: accent hint, authored hint for the first wrong token, general hint, or a generic one.
	 *  Recognize (pick the sentence) gets its own hint: length / highlighted-word hints describe tile building, and
	 *  token hints would point at token 0, which the recognition grader always reports as "first wrong".
	 *  Unspaced languages count tiles, not words (a Japanese chunk like ください is one tile, not "a word").
	 */
	static FText ChooseHint(const FWordTileChallenge& Challenge, const FGradeResult& Result, int32 SubmittedTokens,
		EScaffoldLevel Level = EScaffoldLevel::Assemble, bool bUsesWordSpacing = true);

	/** Display name for UI/telemetry ("Recognize", "GapFill", ...). */
	static FString LevelName(EScaffoldLevel Level);
};
