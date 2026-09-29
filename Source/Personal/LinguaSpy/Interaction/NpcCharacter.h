// LinguaSpy - a local the spy can talk to

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NpcCharacter.generated.h"

class UConversationAsset;
class USphereComponent;
class UStaticMeshComponent;
class UMissionTargetComponent;

/** A conversation the NPC offers when the mission flags match. */
USTRUCT(BlueprintType)
struct FNpcConversationOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TObjectPtr<UConversationAsset> Conversation;

	/** All of these mission flags must be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FName> RequiredFlags;

	/** None of these mission flags may be set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FName> BlockedFlags;
};

/** A local NPC who holds a conversation with the spy when clicked. */
UCLASS()
class ANpcCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ANpcCharacter();

	/** Picks the conversation to play now: the first matching ConversationOptions entry, else Conversation. */
	UFUNCTION(BlueprintPure, Category="NPC")
	UConversationAsset* ResolveConversation() const;

	UFUNCTION(BlueprintPure, Category="NPC")
	const FText& GetDisplayName() const { return DisplayName; }

	/** Id used by mission objectives (TalkTo). */
	UFUNCTION(BlueprintPure, Category="NPC")
	FName GetMissionTargetId() const;

	/** Distance at which the player can start talking. */
	float GetTalkRadius() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	FText DisplayName;

	/** Default conversation when no ConversationOptions entry matches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TObjectPtr<UConversationAsset> Conversation;

	/** Mission-dependent conversations, checked in order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TArray<FNpcConversationOption> ConversationOptions;

	/** Stylized placeholder body (swap for a skeletal mesh in the art pass). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> TalkSphere;

	/** Mission id + beacon shown while this NPC is the current objective. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UMissionTargetComponent> MissionTarget;
};
