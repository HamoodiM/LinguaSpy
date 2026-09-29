// LinguaSpy - conversation panel: NPC line, helper translation, word-tile answer builder

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConversationTypes.h"
#include "ConversationSubsystem.h"
#include "ConversationWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UWrapBox;
class UWidget;
class UWordTileWidget;
class ANpcCharacter;

/**
 *  Conversation UI. Listens to UConversationSubsystem and lets the player build replies from tiles.
 *  Builds a default layout in C++; a WBP subclass may supply its own widgets with the same names.
 */
UCLASS()
class UConversationWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Tile widget class to spawn (defaults to the code-built UWordTileWidget). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversation")
	TSubclassOf<UWordTileWidget> TileClass;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ModeText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> NativeLineText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HelperText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ChallengePanel;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> AnswerRow;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> TileBank;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> HelperToggleButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ClearButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> SubmitButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> LeaveButton;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	void BuildDefaultLayout();
	UButton* MakeButton(const FText& Label, const FName& Name);

	UConversationSubsystem* GetConversationSubsystem() const;

	UFUNCTION()
	void HandleConversationStarted(ANpcCharacter* Npc);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleNodeEntered(const FConversationNode& Node, EConversationMode Mode);

	UFUNCTION()
	void HandleAnswerGraded(const FGradeResult& Result, const FString& ModelAnswer);

	UFUNCTION()
	void HandleCoverBlown();

	UFUNCTION()
	void HandleTileClicked(UWordTileWidget* Tile);

	UFUNCTION()
	void HandleHelperToggle();

	UFUNCTION()
	void HandleClear();

	UFUNCTION()
	void HandleSubmit();

	UFUNCTION()
	void HandleContinue();

	UFUNCTION()
	void HandleLeave();

	void SetButtons(bool bShowSubmit, bool bShowContinue);
	void UpdateHelperVisibility();

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> AnswerTiles;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> BankTiles;

	FText CurrentHelperLine;
	bool bHelperHidden = false;
	bool bTilesLocked = false;
};
