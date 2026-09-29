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

/** Player settings that persist between sessions. */
USTRUCT(BlueprintType)
struct FLinguaSpySettings
{
	GENERATED_BODY()

	/** Show the helper's translation by default. */
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bHelperEnabled = true;

	/** Show romanized readings (romaji) where available. */
	UPROPERTY(BlueprintReadOnly, Category="Settings")
	bool bShowReadings = true;
};

UCLASS()
class ULinguaSpySaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = CurrentVersion;

	UPROPERTY()
	TMap<FName, FSkillStats> SkillStats;

	UPROPERTY()
	TMap<FName, FCityProgress> Cities;

	UPROPERTY()
	FLinguaSpySettings Settings;
};
