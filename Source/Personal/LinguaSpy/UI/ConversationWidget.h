// LinguaSpy - conversation panel: NPC line, helper translation, choices, word-tile answer builder

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
class ULanguagePackAsset;
class ULinguaSpySaveSubsystem;

/**
 *  Conversation UI. Listens to UConversationSubsystem and lets the player pick choices and build replies from tiles.
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

	/** Romanized reading of the NPC line (only for languages with readings). */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadingLineText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HelperText;

	/** Holds one button per option at a PlayerChoice node. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> ChoiceBox;

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
	TObjectPtr<UButton> ReadingToggleButton;

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
	ULinguaSpySaveSubsystem* GetSaveSubsystem() const;

	UFUNCTION()
	void HandleConversationStarted(ANpcCharacter* Npc);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleNodeEntered(const FConversationNode& Node, EConversationMode Mode);

	UFUNCTION()
	void HandleChallengeReady(const FWordTileChallenge& Challenge);

	UFUNCTION()
	void HandleAnswerGraded(const FGradeResult& Result, const FString& ModelAnswer);

	UFUNCTION()
	void HandleCoverBlown();

	UFUNCTION()
	void HandleTileClicked(UWordTileWidget* Tile);

	UFUNCTION()
	void HandleChoiceClicked(UWordTileWidget* ChoiceButton);

	UFUNCTION()
	void HandleHelperToggle();

	UFUNCTION()
	void HandleReadingToggle();

	UFUNCTION()
	void HandleClear();

	UFUNCTION()
	void HandleSubmit();

	UFUNCTION()
	void HandleContinue();

	UFUNCTION()
	void HandleLeave();

	void SetButtons(bool bShowSubmit, bool bShowContinue);
	void ClearTiles();
	void ClearChoices();
	void RefreshHelperAndReadings();
	void ApplyNativeFont(UTextBlock* Text, int32 Size) const;

	bool IsHelperEnabled() const;
	bool ShouldShowReadings() const;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> AnswerTiles;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> BankTiles;

	/** Choice button -> choice index at the current node. */
	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, int32> ChoiceButtons;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> ActiveLanguage;

	FText CurrentHelperLine;
	FText CurrentReadingLine;
	FWordTileChallenge CurrentChallenge;
	bool bTilesLocked = false;
};
