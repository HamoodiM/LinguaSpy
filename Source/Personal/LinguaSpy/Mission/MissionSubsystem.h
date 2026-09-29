// LinguaSpy - tracks the active city, mission, objective and story flags

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MissionAsset.h"
#include "LinguaSpySaveGame.h"
#include "MissionSubsystem.generated.h"

class ULanguagePackAsset;
class ULinguaSpySaveSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnObjectiveChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEvent, UMissionAsset*, Mission);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCityCompleted);

/**
 *  Game-instance-wide mission state. The city game mode calls EnterCityForWorld; conversations, markers and
 *  pickups report back via Report*; objectives advance via UMissionAsset::ResolveTransition.
 *  Progress is stored in the save profile.
 */
UCLASS()
class UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ---- City selection -------------------------------------------------------------------------------------

	/** All language packs, sorted by SortOrder. Loads them synchronously. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Mission")
	TArray<ULanguagePackAsset*> GetAllLanguagePacks() const;

	/** Remembers the pack chosen in the menu (used when its map loads). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Mission")
	void SelectLanguagePack(ULanguagePackAsset* Pack);

	/** Called by the city game mode. Picks the pack for this map and resumes/starts its mission. */
	void EnterCityForWorld(UWorld* World);

	/** Called when leaving a city (e.g. back to the menu). */
	void LeaveCity();

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	ULanguagePackAsset* GetCurrentPack() const { return CurrentPack; }

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	FName GetCurrentCityId() const;

	/** Completed / total missions for a city (for the menu). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	void GetCityCompletion(const ULanguagePackAsset* Pack, int32& OutCompleted, int32& OutTotal) const;

	// ---- Mission state --------------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	UMissionAsset* GetActiveMission() const { return ActiveMission; }

	/** Current objective, or nullptr if none (no city / all missions done). */
	const FMissionObjective* GetCurrentObjective() const;

	/** TargetId of the current objective (None if no objective). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	FName GetCurrentTargetId() const;

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	FText GetCurrentObjectiveText() const;

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	bool HasFlag(FName Flag) const;

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Mission")
	void SetFlag(FName Flag);

	/** True if every flag is set. */
	bool HasAllFlags(const TArray<FName>& Flags) const;

	/** True if any flag is set. */
	bool HasAnyFlag(const TArray<FName>& Flags) const;

	// ---- Reports from the world -----------------------------------------------------------------------------

	void ReportConversationFinished(FName NpcTargetId, FName Outcome);
	void ReportMarkerReached(FName MarkerTargetId);
	void ReportCollected(FName PickupTargetId);

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Mission")
	FOnObjectiveChanged OnObjectiveChanged;

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Mission")
	FOnMissionEvent OnMissionStarted;

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Mission")
	FOnMissionEvent OnMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Mission")
	FOnMissionEvent OnMissionFailed;

	UPROPERTY(BlueprintAssignable, Category="LinguaSpy|Mission")
	FOnCityCompleted OnCityCompleted;

private:

	FCityProgress* GetProgress() const;
	void StartNextMission();
	void StartMission(UMissionAsset* Mission);
	void CompleteObjective(FName Outcome);
	void SaveProgress();

	UPROPERTY()
	TObjectPtr<ULinguaSpySaveSubsystem> SaveSubsystem;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> SelectedPack;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> CurrentPack;

	UPROPERTY()
	TObjectPtr<UMissionAsset> ActiveMission;
};
