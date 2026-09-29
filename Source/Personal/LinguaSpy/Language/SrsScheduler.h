// LinguaSpy - spaced-repetition scheduling (Leitner boxes on real time)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LinguaSpySaveGame.h"
#include "SrsScheduler.generated.h"

/** Identifies one reviewable challenge: a conversation node (plus the choice index for PlayerChoice nodes). */
USTRUCT(BlueprintType)
struct PERSONAL_API FSrsItemKey
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FSoftObjectPath Conversation;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FName NodeId;

	/** INDEX_NONE for PlayerChallenge nodes. */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int32 ChoiceIndex = INDEX_NONE;

	bool IsValid() const { return Conversation.IsValid() && !NodeId.IsNone(); }

	/** Stable save key: "<ConversationPath>#<NodeId>#<ChoiceIndex>". */
	FString ToString() const;

	/** Inverse of ToString. Returns false for malformed keys. */
	static bool Parse(const FString& Key, FSrsItemKey& OutKey);

	bool operator==(const FSrsItemKey& Other) const
	{
		return Conversation == Other.Conversation && NodeId == Other.NodeId && ChoiceIndex == Other.ChoiceIndex;
	}
};

/**
 *  Pure, testable Leitner scheduling.
 *  Box 0 = seen, never passed unaided. Boxes 1..5 wait 10 min, 1 day, 3 days, 7 days, 21 days.
 *  Unaided pass -> up one box. Pass after peeking at the helper -> same box, rescheduled. Fail -> box 1, due in 10 min.
 */
UCLASS()
class PERSONAL_API USrsScheduler : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static constexpr int32 MaxBox = 5;

	/** Wait after reaching a box. */
	static FTimespan GetInterval(int32 Box);

	/** Applies one answer to the item's schedule. */
	static void Update(FSrsItemState& State, bool bPassed, bool bAssisted, const FDateTime& Now);

	static bool IsDue(const FSrsItemState& State, const FDateTime& Now);

	/** 0..1 for UI strength dots (Box / MaxBox). */
	static float GetStrength(const FSrsItemState& State);

	/**
	 *  Picks up to Max item keys from one city (NAME_None = any city).
	 *  Due items first (most overdue, then lowest box). If bFillWithNotDue, tops up with the weakest non-due items.
	 */
	static TArray<FString> SelectReviewItems(const TMap<FString, FSrsItemState>& Items, FName CityId, const FDateTime& Now, int32 Max, bool bFillWithNotDue);

	/** Number of due items in a city (NAME_None = any city). */
	static int32 CountDue(const TMap<FString, FSrsItemState>& Items, FName CityId, const FDateTime& Now);
};
