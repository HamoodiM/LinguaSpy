// LinguaSpy - tracks the active city, mission, objective and story flags

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MissionAsset.h"
#include "LinguaSpySaveGame.h"
#include "MissionSubsystem.generated.h"

class ULanguagePackAsset;
class ULinguaSpySaveSubsystem;

struct FSrsItemKey;

/** One challenge answered during the active mission (feeds the debrief). */
struct FMissionItemRecord
{
	FString Key;
	bool bPassed = false;
	bool bAssisted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnObjectiveChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEvent, UMissionAsset*, Mission);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCityCompleted);

/**
 *  Game-instance-wide mission state. The city game mode calls EnterCityForWorld; conversations, markers and
 *  pickups report back via Report*; objectives advance via UMissionAsset::ResolveTransition.
 *  Progress is stored in the save profile.
 */
UCLASS()
class PERSONAL_API UMissionSubsystem : public UGameInstanceSubsystem
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

	// ---- Fluency (helper reliance) ---------------------------------------------------------------------------

	/** Adds a finished NPC conversation's tallies to the active mission's fluency grade. */
	void AddFluencyTally(int32 Challenges, int32 Mistakes, int32 Peeks);

	/** Tallies so far for the active mission (reset when a mission starts, restarts or the city is re-entered). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	FMissionGrade GetCurrentMissionTally() const { return MissionTally; }

	/** Grade of the most recently completed mission (valid during/after OnMissionCompleted). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	FMissionGrade GetLastMissionGrade() const { return LastGrade; }

	/** Best grade ever achieved in a mission. Returns false if it was never completed. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	bool GetBestGrade(FName MissionId, FMissionGrade& OutGrade) const;

	// ---- Learning loop (briefing -> field -> debrief) ---------------------------------------------------------

	/** Records a graded challenge for the debrief (called by the conversation subsystem). */
	void RecordMissionItem(const FSrsItemKey& Key, bool bPassed, bool bAssisted);

	/**
	 *  Pure: debrief picks - failed items first, then assisted ones, then the rest (in answer order), unique, up to Max.
	 *  Exposed for tests.
	 */
	static TArray<FString> SelectDebriefItems(const TArray<FMissionItemRecord>& Records, int32 Max);

	/** Pure: handler support fades after a strong grade (S/A: one step less help) and returns after a weak one (C). */
	static EHandlerMode AdaptHandlerMode(EHandlerMode Current, int32 Grade);

	/** True if the mission has briefing chunks the player hasn't been through yet. */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	bool NeedsBriefing(const UMissionAsset* Mission) const;

	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Mission")
	void MarkBriefingCompleted(const UMissionAsset* Mission, int32 Correct, int32 Total);

	/** Handler mode before/after the last mission's adaptation (equal if unchanged). */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Mission")
	void GetLastHandlerChange(EHandlerMode& OutBefore, EHandlerMode& OutAfter) const { OutBefore = HandlerBefore; OutAfter = HandlerAfter; }

	/** Can-do statements of every completed mission in a city. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Mission")
	TArray<FText> GetEarnedCanDos(const ULanguagePackAsset* Pack) const;

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

	FMissionGrade MissionTally;
	FMissionGrade LastGrade;

	TArray<FMissionItemRecord> MissionItems;
	EHandlerMode HandlerBefore = EHandlerMode::Shadowing;
	EHandlerMode HandlerAfter = EHandlerMode::Shadowing;
};
