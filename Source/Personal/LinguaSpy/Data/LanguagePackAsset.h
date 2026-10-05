// LinguaSpy - language / city pack

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LanguagePackAsset.generated.h"

class UConversationAsset;
class UMissionAsset;
class UFont;
class USoundBase;

/**
 *  A short explicit grammar note ("field note"), shown once the learner has met the pattern a few times
 *  (noticing + brief explicit instruction: docs/Learning_Design.md section 4).
 */
USTRUCT(BlueprintType)
struct PERSONAL_API FGrammarNote
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grammar")
	FName NoteId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grammar")
	FText Title;

	/** 2-4 lines: the pattern, then examples the player has already seen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grammar", meta=(MultiLine=true))
	FText Body;

	/** A graded answer containing any of these tokens counts as an encounter (case-insensitive, e.g. "voudrais"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grammar")
	TArray<FString> TriggerTokens;

	/** Encounters before the note pops up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grammar", meta=(ClampMin="1"))
	int32 TriggerAfter = 2;
};

/** Describes one playable city and the language learned there (e.g. Paris / French). */
UCLASS(BlueprintType)
class PERSONAL_API ULanguagePackAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Primary asset type used by the menu to discover cities. */
	static const FPrimaryAssetType PrimaryAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Stable id used for save data and skill keys (e.g. "Paris"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FName CityId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FText LanguageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FText CityName;

	/** Culture code, e.g. "fr", "es", "ja", "it". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FString LanguageCode;

	/** Short tagline shown on the city card. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language", meta=(MultiLine=true))
	FText Tagline;

	/** False for languages written without spaces between words (e.g. Japanese). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	bool bUsesWordSpacing = true;

	/** True if lines/tiles carry readings (romaji); enables the romaji toggle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	bool bHasReadings = false;

	/** Allows the typing scaffolds (gap-fill / produce). Only used when bUsesWordSpacing (no IME support yet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	bool bAllowTypedInput = true;

	/** Field notes for this language. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Learning", meta=(TitleProperty="Title"))
	TArray<FGrammarNote> GrammarNotes;

	bool CanType() const { return bAllowTypedInput && bUsesWordSpacing; }

	/** Optional font for target-language text (e.g. a CJK font). Uses the default font if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TSoftObjectPtr<UFont> NativeFont;

	/** Card colour on the city-select screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	FLinearColor AccentColor = FLinearColor(0.9f, 0.6f, 0.3f);

	/** Order on the city-select screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TSoftObjectPtr<UWorld> CityMap;

	/** Missions played in order in this city. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TArray<TSoftObjectPtr<UMissionAsset>> Missions;

	/** Looping street ambience played in the city. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<USoundBase> AmbientSound;

	/** Looping music played in the city. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<USoundBase> MusicSound;

	/** Conversations that belong to this city (for tooling). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TArray<TSoftObjectPtr<UConversationAsset>> Conversations;
};
