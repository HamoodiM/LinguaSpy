// LinguaSpy - per-skill learning progress

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LinguaSpySaveGame.h"
#include "LanguageProgressSubsystem.generated.h"

class ULinguaSpySaveSubsystem;

/**
 *  Tracks how well the player does per language skill (Greetings, Ordering, ...), per city.
 *  Stats live in the save profile; keys are "<CityId>.<SkillTag>".
 */
UCLASS()
class ULanguageProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Progress")
	void RecordResult(FName CityId, FName SkillTag, bool bPassed);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Progress")
	FSkillStats GetStats(FName CityId, FName SkillTag) const;

	/** Skills in a city where failures outnumber passes, worst first. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Progress")
	TArray<FName> GetWeakSkills(FName CityId) const;

	static FName MakeKey(FName CityId, FName SkillTag);

private:

	TMap<FName, FSkillStats>* GetStatsMap() const;

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;
};
