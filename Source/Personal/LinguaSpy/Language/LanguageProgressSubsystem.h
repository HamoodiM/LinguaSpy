// LinguaSpy - per-skill learning progress

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LanguageProgressSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSkillStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	int32 Passes = 0;

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	int32 Failures = 0;
};

/**
 *  Tracks how well the player does per language skill (Greetings, Ordering, ...).
 *  Runtime only for now; persisted via USaveGame in Phase 2.
 */
UCLASS()
class ULanguageProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Progress")
	void RecordResult(FName SkillTag, bool bPassed);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Progress")
	FSkillStats GetStats(FName SkillTag) const;

	/** Skills where failures outnumber passes, worst first. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Progress")
	TArray<FName> GetWeakSkills() const;

private:

	UPROPERTY()
	TMap<FName, FSkillStats> Stats;
};
