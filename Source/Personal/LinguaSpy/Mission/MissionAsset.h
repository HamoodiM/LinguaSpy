// LinguaSpy - branching spy missions

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionAsset.generated.h"

class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EObjectiveType : uint8
{
	/** Finish a conversation with the NPC whose MissionTargetId == TargetId. */
	TalkTo,
	/** Walk into the AMissionMarker whose MissionTargetId == TargetId. */
	ReachLocation,
	/** Pick up the AMissionPickup whose MissionTargetId == TargetId. */
	Collect
};

/** Where to go when an objective finishes. The first matching transition wins. */
USTRUCT(BlueprintType)
struct FObjectiveTransition
{
	GENERATED_BODY()

	/** Only matches this outcome (conversation End outcome, "Reached", "Collected"). None matches any outcome. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	FName RequiredOutcome;

	/** All of these mission flags must be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	TArray<FName> RequiredFlags;

	/** None of these mission flags may be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	TArray<FName> BlockedFlags;

	/** Next objective id, or MissionComplete / MissionFailed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	FName NextObjective;
};

USTRUCT(BlueprintType)
struct FMissionObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	EObjectiveType Type = EObjectiveType::TalkTo;

	/** MissionTargetId of the NPC / marker / pickup this objective points at. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	FName TargetId;

	/** Shown in the HUD tracker (player's native language). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	FText Description;

	/** Flags set when this objective completes (before transitions are evaluated). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	TArray<FName> SetFlags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective", meta=(TitleProperty="NextObjective"))
	TArray<FObjectiveTransition> Transitions;
};

/**
 *  A target phrase taught in the mission briefing before the player has to use it
 *  (comprehensible input before output: docs/Learning_Design.md section 1).
 */
USTRUCT(BlueprintType)
struct FBriefingChunk
{
	GENERATED_BODY()

	/** The phrase in the target language (e.g. "je voudrais"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	FString Text;

	/** Optional reading (romaji). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	FString Reading;

	/** Meaning in the player's language (e.g. "I would like"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	FText Meaning;

	/** A situation where you'd say it (e.g. "You want to order politely."), used by the "Which would you say?" step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	FText Situation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	TSoftObjectPtr<USoundBase> Audio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Briefing")
	FName SkillTag;
};

/** A spy mission: a graph of objectives connected by outcome/flag-based transitions. */
UCLASS(BlueprintType)
class PERSONAL_API UMissionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	static const FName MissionComplete;
	static const FName MissionFailed;
	static const FName OutcomeReached;
	static const FName OutcomeCollected;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName MissionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FText Title;

	/** Handler briefing shown when the mission starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission", meta=(MultiLine=true))
	FText Briefing;

	/** Target phrases taught before the mission (4-8). Empty = no briefing screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Learning", meta=(TitleProperty="Text"))
	TArray<FBriefingChunk> BriefingChunks;

	/** CEFR "can-do" statements this mission trains (e.g. "Can order food and drink politely" - A1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Learning")
	TArray<FText> CanDoStatements;

	/** Number of retrieval items in the post-mission debrief (0 = no debrief). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Learning", meta=(ClampMin="0", ClampMax="10"))
	int32 DebriefItems = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName StartObjective;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission", meta=(TitleProperty="ObjectiveId"))
	TArray<FMissionObjective> Objectives;

	const FMissionObjective* FindObjective(FName ObjectiveId) const;

	/**
	 *  Pure transition logic: returns the next objective id for a finished objective, or None if no transition
	 *  matches (the objective stays active).
	 */
	static FName ResolveTransition(const FMissionObjective& Objective, FName Outcome, const TSet<FName>& Flags);

	/** Validation problems (bad links, missing targets...). Empty if valid. */
	UFUNCTION(BlueprintCallable, Category="Mission")
	TArray<FText> Validate() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
