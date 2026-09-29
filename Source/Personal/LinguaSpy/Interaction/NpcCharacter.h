// LinguaSpy - a local the spy can talk to

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NpcCharacter.generated.h"

class UConversationAsset;
class USphereComponent;
class UStaticMeshComponent;

/** A local NPC who holds a conversation with the spy when clicked. */
UCLASS()
class ANpcCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ANpcCharacter();

	UFUNCTION(BlueprintPure, Category="NPC")
	UConversationAsset* GetConversation() const { return Conversation; }

	UFUNCTION(BlueprintPure, Category="NPC")
	const FText& GetDisplayName() const { return DisplayName; }

	/** Distance at which the player can start talking. */
	float GetTalkRadius() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC")
	TObjectPtr<UConversationAsset> Conversation;

	/** Stylized placeholder body (swap for a skeletal mesh in the art pass). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> TalkSphere;
};
