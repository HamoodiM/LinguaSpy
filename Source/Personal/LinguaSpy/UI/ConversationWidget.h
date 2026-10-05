// LinguaSpy - conversation panel: NPC line, helper translation, choices, scaffolded answer input, intel checks

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConversationTypes.h"
#include "ConversationSubsystem.h"
#include "ConversationWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UWrapBox;
class UVerticalBox;
class UWidget;
class UWordTileWidget;
class ANpcCharacter;
class ULanguagePackAsset;
class ULinguaSpySaveSubsystem;

/**
 *  Conversation UI. Listens to UConversationSubsystem and lets the player pick choices and answer challenges at their
 *  scaffold level: pick a sentence (Recognize), build it from tiles (Assemble / AssemblePlus), type the missing words
 *  (GapFill) or type it all (Produce). Builds a default layout in C++; a WBP subclass may supply widgets with the same names.
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

	/** Holds one button per option at a PlayerChoice node, or the answers of an intel check. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> ChoiceBox;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ChallengePanel;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentText;

	/** Which support level this challenge has ("Words given", "Type the missing word"...). */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** GapFill: the sentence with blanks. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GapText;

	/** Recognize: one button per candidate sentence. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionBox;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> AnswerArea;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> AnswerRow;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> TileBank;

	/** GapFill / Produce: typed answer. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UEditableTextBox> TypeBox;

	/** Accent keys under the typing box (from ULanguagePackAsset::SpecialCharacters). */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> AccentBar;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> HelperToggleButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ReadingToggleButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> RepeatButton;

	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UButton> SlowButton;

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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** The panel that slides in when a conversation starts. */
	UPROPERTY(BlueprintReadOnly, Category="Conversation", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Panel;

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
	void HandleChallengeReady(const FChallengePresentation& Presentation);

	UFUNCTION()
	void HandleAnswerFeedback(const FAnswerFeedback& Feedback);

	UFUNCTION()
	void HandleIntelAnswered(bool bCorrect, int32 ChoiceIndex);

	UFUNCTION()
	void HandleCoverBlown();

	UFUNCTION()
	void HandleTileClicked(UWordTileWidget* Tile);

	UFUNCTION()
	void HandleChoiceClicked(UWordTileWidget* ChoiceButton);

	UFUNCTION()
	void HandleOptionClicked(UWordTileWidget* OptionButton);

	UFUNCTION()
	void HandleIntelClicked(UWordTileWidget* OptionButton);

	UFUNCTION()
	void HandleHelperToggle();

	UFUNCTION()
	void HandleReadingToggle();

	UFUNCTION()
	void HandleRepeat();

	UFUNCTION()
	void HandleSlow();

	UFUNCTION()
	void HandleClear();

	UFUNCTION()
	void HandleSubmit();

	UFUNCTION()
	void HandleTypeCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleAccentKey(UWordTileWidget* Key);

	void RebuildAccentBar(bool bVisible);

	UFUNCTION()
	void HandleContinue();

	UFUNCTION()
	void HandleLeave();

	void SetButtons(bool bShowSubmit, bool bShowContinue);
	void ClearTiles();
	void ClearChoices();
	void ClearOptions();
	void RefreshHelperAndReadings();
	void RefreshAudioButtons();
	void ShowFeedback(const FText& Text, const FLinearColor& Color);
	void ApplyNativeFont(UTextBlock* Text, int32 Size) const;
	UWordTileWidget* MakeOptionButton(const FString& Label, bool bNativeText);
	bool IsTypingLevel() const;

	/** Whether the helper's translation of the current line is visible (tapped Translate, or Shadowing mode). */
	bool IsHelperEnabled() const;
	void SetButtonLabel(UButton* Button, const FText& Label) const;
	bool ShouldShowReadings() const;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> AnswerTiles;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> BankTiles;

	/** Choice button -> choice index at the current node. */
	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, int32> ChoiceButtons;

	/** Recognition / intel option button -> option index. */
	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, int32> OptionButtons;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> ActiveLanguage;

	FText CurrentHelperLine;
	FText CurrentReadingLine;
	FChallengePresentation CurrentPresentation;
	bool bTilesLocked = false;
	bool bIntelNode = false;
	float SlideTime = -1.f;
};
