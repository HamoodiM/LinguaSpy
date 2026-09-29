// LinguaSpy - persistent player profile

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LinguaSpySaveGame.generated.h"

/** Per-skill practice statistics. Keyed "<CityId>.<SkillTag>". */
USTRUCT(BlueprintType)
struct FSkillStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	int32 Passes = 0;

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	int32 Failures = 0;
};

/** Mission progress in one city. */
USTRUCT(BlueprintType)
struct FCityProgress
{
	GENERATED_BODY()

	/** Mission currently being played (None = pick the next uncompleted one). */
	UPROPERTY(BlueprintReadOnly, Category="Progress")
	FName ActiveMission;

	/** Current objective of the active mission. */
	UPROPERTY(BlueprintReadOnly, Category="Progress")
	FName CurrentObjective;

	/** Story flags set during the active mission (cleared when a mission starts or fails). */
	UPROPERTY(BlueprintReadOnly, Category="Progress")
	TSet<FName> MissionFlags;

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	TArray<FName> CompletedMissions;
};

/** Spaced-repetition schedule for one challenge (Leitner box + due time). Keyed by FSrsItemKey::ToString(). */
USTRUCT(BlueprintType)
struct FSrsItemState
{
	GENERATED_BODY()

	/** 0 = seen but never passed unassisted; 1..5 = Leitner boxes (higher = longer interval). */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int32 Box = 0;

	/** When the item is next due for review (UTC ticks). */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int64 DueUtcTicks = 0;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int64 LastReviewUtcTicks = 0;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int32 Passes = 0;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int32 Failures = 0;

	/** Passes given after peeking at the helper's translation. */
	UPROPERTY(BlueprintReadOnly, Category="SRS")
	int32 AssistedPasses = 0;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FName CityId;

	UPROPERTY(BlueprintReadOnly, Category="SRS")
	FName SkillTag;
};

/** Fluency grade for one mission (0 = S, 1 = A, 2 = B, 3 = C). */
USTRUCT(BlueprintType)
struct FMissionGrade
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 Grade = 3;

	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 Challenges = 0;

	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 Mistakes = 0;

	UPROPERTY(BlueprintReadOnly, Category="Grade")
	int32 Peeks = 0;
};

/** Player settings that persist between sessions. */
USTRUCT(BlueprintType)
struct FLinguaSpySettings
{
	GENERATED_BODY()

	/** Show the helper's translation on every line without tapping (answers then count as assisted). */
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bAlwaysShowTranslation = false;

	/** Show romanized readings (romaji) where available. */
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bShowReadings = true;

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	float MasterVolume = 0.8f;

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	float MusicVolume = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	float AmbienceVolume = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Settings")
	float UIVolume = 0.7f;
};

UCLASS()
class PERSONAL_API ULinguaSpySaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	/** v1: skills, cities, settings. v2: SRS items, best mission grades, volume + translation settings. */
	static constexpr int32 CurrentVersion = 2;

	/** Upgrades an older profile in place, one version at a time. Returns true if anything changed. */
	static bool Migrate(ULinguaSpySaveGame* Save);

	/**
	 *  Format version of this profile. Defaults to 0 on purpose: SaveGame serialization skips properties equal to the
	 *  class default, so a default of CurrentVersion would never be written and old files would load as "current".
	 *  Files written before v2 therefore load as 0 (= v1). Stamped with CurrentVersion on save / creation.
	 */
	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	TMap<FName, FSkillStats> SkillStats;

	UPROPERTY()
	TMap<FName, FCityProgress> Cities;

	UPROPERTY()
	FLinguaSpySettings Settings;

	/** Spaced-repetition items, keyed by FSrsItemKey::ToString(). */
	UPROPERTY()
	TMap<FString, FSrsItemState> SrsItems;

	/** Best fluency grade per MissionId. */
	UPROPERTY()
	TMap<FName, FMissionGrade> BestGrades;
};
