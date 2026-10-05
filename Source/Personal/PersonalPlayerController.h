// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
//#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "LanguagePackAsset.h"
#include "PersonalPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class ANpcCharacter;
class UConversationWidget;
class USpyHUDWidget;
class UPauseMenuWidget;
class UMissionAsset;
class UBriefingWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Player controller for a top-down perspective game.
 *  Implements point and click based controls
 */
UCLASS(abstract)
class APersonalPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Component used for moving along a NavMesh path. */
	UPROPERTY(VisibleDefaultsOnly, Category = AI)
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, Category="Input")
	float ShortPressThreshold;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	/** MappingContext */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationClickAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationTouchAction;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Set to true if we're using touch input */
	uint32 bIsTouch : 1;

	/** Saved location of the character movement destination */
	FVector CachedDestination;

	/** Time that the click input has been pressed */
	float FollowTime = 0.0f;

	/** LinguaSpy: conversation UI class (defaults to the code-built layout) */
	UPROPERTY(EditAnywhere, Category="LinguaSpy")
	TSubclassOf<UConversationWidget> ConversationWidgetClass;

	/** LinguaSpy: HUD class (defaults to the code-built layout) */
	UPROPERTY(EditAnywhere, Category="LinguaSpy")
	TSubclassOf<USpyHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UConversationWidget> ConversationWidget;

	UPROPERTY(Transient)
	TObjectPtr<USpyHUDWidget> HUDWidget;

	/** LinguaSpy: pause menu class (defaults to the code-built layout) */
	UPROPERTY(EditAnywhere, Category="LinguaSpy")
	TSubclassOf<UPauseMenuWidget> PauseMenuClass;

	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenu;

	/** NPC we are walking towards to talk to */
	TWeakObjectPtr<ANpcCharacter> PendingTalkTarget;

	/** True if the current click was on an NPC, so it should not also be treated as a move command */
	bool bClickTargetedNpc = false;

public:

	/** Constructor */
	APersonalPlayerController();

protected:

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PlayerTick(float DeltaTime) override;

	/** True while a conversation is running (movement input is ignored) */
	bool IsInConversation() const;

	UFUNCTION()
	void HandleConversationStarted(ANpcCharacter* Npc);

	UFUNCTION()
	void HandleConversationEnded();

	/** LinguaSpy: shows the mission briefing (phrases first) when a mission needs one, once no conversation is running */
	UFUNCTION()
	void HandleMissionStarted(UMissionAsset* Mission);

	void TryOpenBriefing();

	UFUNCTION()
	void HandleBriefingClosed();

	UFUNCTION()
	void HandleFieldNote(const FGrammarNote& Note);

	/** Shows the next queued field note (one card at a time). */
	UFUNCTION()
	void ShowNextFieldNote();

	TArray<FGrammarNote> PendingFieldNotes;
	bool bFieldNoteOpen = false;

	UPROPERTY(Transient)
	TObjectPtr<UBriefingWidget> BriefingWidget;

	bool bBriefingPending = false;

	/** Opens/closes the pause menu (P or Escape) */
	UFUNCTION()
	void TogglePause();
	
	/** Input handlers */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();

	/** Helper function to get the move destination */
	void UpdateCachedDestination();
};


