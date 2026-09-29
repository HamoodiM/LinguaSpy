// LinguaSpy - language / city pack

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LanguagePackAsset.generated.h"

class UConversationAsset;
class UMissionAsset;
class UFont;

/** Describes one playable city and the language learned there (e.g. Paris / French). */
UCLASS(BlueprintType)
class ULanguagePackAsset : public UPrimaryDataAsset
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

	/** Conversations that belong to this city (for tooling). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language")
	TArray<TSoftObjectPtr<UConversationAsset>> Conversations;
};
