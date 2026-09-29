// LinguaSpy - spaced-repetition items stored in the save profile

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SrsScheduler.h"
#include "SpacedRepetitionSubsystem.generated.h"

class ULinguaSpySaveSubsystem;
class UConversationAsset;

/** A resolved SRS item for UI (phrasebook, practice screen). */
USTRUCT(BlueprintType)
struct PERSONAL_API FSrsItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FSrsItemKey Key;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FSrsItemState State;

	/** What the phrase means (the challenge's intent prompt, in the player's language). */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FText Meaning;

	/** The first accepted answer, joined for display. */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FString Phrase;

	/** Readings of the answer tiles (romaji), if the language has them. */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FString Reading;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	bool bDue = false;
};

/**
 *  Records every graded answer against its challenge and schedules reviews (USrsScheduler).
 *  Items live in ULinguaSpySaveGame::SrsItems; "now" is real UTC time plus an optional debug offset.
 */
UCLASS()
class PERSONAL_API USpacedRepetitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RecordResult(const FSrsItemKey& Key, FName CityId, FName SkillTag, bool bPassed, bool bAssisted);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|SRS")
	int32 GetDueCount(FName CityId) const;

	/** Number of items the player has passed at least once in a city. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|SRS")
	int32 GetLearnedCount(FName CityId) const;

	/** Due items first; when fewer than Max are due, tops up with the weakest items ("extra practice"). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|SRS")
	TArray<FSrsItemKey> BuildReviewQueue(FName CityId, int32 Max = 10) const;

	/** Every item the player has passed at least once in a city, resolved for display, grouped by skill. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|SRS")
	TArray<FSrsItemView> GetLearnedItems(FName CityId) const;

	/** Average box strength (0..1) per skill in a city, for the practice screen. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|SRS")
	TMap<FName, float> GetSkillStrengths(FName CityId) const;

	/** Current time used for scheduling (UTC + debug offset). */
	FDateTime GetNow() const { return FDateTime::UtcNow() + DebugTimeOffset; }

	/** Debug / automation: pretend this many hours have passed (e.g. to make items due). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|SRS")
	void SetDebugTimeOffsetHours(float Hours) { DebugTimeOffset = FTimespan::FromHours(Hours); }

	/** Resolves an item into display text. Returns false if its conversation/node no longer exists. */
	static bool ResolveItem(const FSrsItemKey& Key, FSrsItemView& OutView);

private:

	TMap<FString, FSrsItemState>* GetItems() const;

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;

	FTimespan DebugTimeOffset;
};
