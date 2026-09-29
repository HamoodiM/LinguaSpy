// LinguaSpy - a local the spy can talk to

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NpcCharacter.generated.h"

class UConversationAsset;
class USphereComponent;
class UStaticMeshComponent;
class UMissionTargetComponent;

/** A conversation the NPC offers when the mission state matches. */
USTRUCT(BlueprintType)
struct FNpcConversationOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TObjectPtr<UConversationAsset> Conversation;

	/** Only offered while this mission (MissionId) is active. None = any mission (or none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	FName RequiredMission;

	/** Only offered while this NPC is the current objective's target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	bool bOnlyIfCurrentTarget = false;

	/** All of these mission flags must be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FName> RequiredFlags;

	/** None of these mission flags may be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FName> BlockedFlags;
};

/** A local NPC who holds a conversation with the spy when clicked, and turns to face them while talking. */
UCLASS()
class ANpcCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ANpcCharacter();

	/** Picks the conversation to play now: the first matching ConversationOptions entry, else Conversation. */
	UFUNCTION(BlueprintPure, Category="NPC")
	UConversationAsset* ResolveConversation() const;

	/**
	 *  Pure option selection (testable): index of the first option matching the mission state, or INDEX_NONE.
	 */
	static int32 ChooseConversationOption(const TArray<FNpcConversationOption>& Options, FName ActiveMissionId,
		bool bIsCurrentTarget, TFunctionRef<bool(FName)> HasFlag);

	UFUNCTION(BlueprintPure, Category="NPC")
	const FText& GetDisplayName() const { return DisplayName; }

	/** Id used by mission objectives (TalkTo). */
	UFUNCTION(BlueprintPure, Category="NPC")
	FName GetMissionTargetId() const;

	/** Distance at which the player can start talking. */
	float GetTalkRadius() const;

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	FText DisplayName;

	/** Default conversation when no ConversationOptions entry matches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TObjectPtr<UConversationAsset> Conversation;

	/** Mission-dependent conversations, checked in order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FNpcConversationOption> ConversationOptions;

	/** Degrees per second when turning to face the player. */
	UPROPERTY(EditAnywhere, Category="NPC")
	float TurnSpeed = 300.f;

	/** Fallback placeholder body, only shown when the skeletal mesh has no asset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> TalkSphere;

	/** Mission id + beacon shown while this NPC is the current objective. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UMissionTargetComponent> MissionTarget;

private:

	UFUNCTION()
	void HandleConversationStarted(ANpcCharacter* Npc);

	UFUNCTION()
	void HandleConversationEnded();

	void UpdateBodyVisibility();

	float HomeYaw = 0.f;
	float TargetYaw = 0.f;
	bool bTalking = false;
};
