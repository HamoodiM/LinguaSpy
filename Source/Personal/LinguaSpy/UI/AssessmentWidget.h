// LinguaSpy - assessment overlay: participant code, pre/post/delayed tests, placement (no feedback per item)

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "AssessmentWidget.generated.h"

class UEditableTextBox;
class ULanguagePackAsset;
class UVerticalBox;
class UWrapBox;
class UWordTileWidget;
class UAssessmentSubsystem;

/**
 *  Pilot mode: enter a participant code (never a name), then take the pre-test, post-test or 1-week test; the form is
 *  counterbalanced by UAssessmentSubsystem. Outside pilot mode: "Check my level" (placement) for a language.
 *  Items show no feedback; the score is only shown outside pilot mode.
 */
UCLASS()
class UAssessmentWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	virtual void Close() override;

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual float GetPanelWidth() const override { return 820.f; }

private:

	void ShowHome();
	void ShowCodeEntry();
	void ShowItem();
	void ShowDone();
	void ClearBody();
	void StartPhase(FName Phase);
	bool IsPilot() const;
	UAssessmentSubsystem* GetAssessment() const;
	UButton* AddButtonRow(const FText& Label);

	UFUNCTION()
	void HandleCodeSave();

	UFUNCTION()
	void HandlePre();

	UFUNCTION()
	void HandlePost();

	UFUNCTION()
	void HandleDelayed();

	UFUNCTION()
	void HandlePlacement();

	UFUNCTION()
	void HandleLanguage();

	UFUNCTION()
	void HandleChoice(UWordTileWidget* Option);

	UFUNCTION()
	void HandleTile(UWordTileWidget* Tile);

	UFUNCTION()
	void HandleHear();

	UFUNCTION()
	void HandleNext();

	UFUNCTION()
	void HandleOpenLogs();

	UFUNCTION()
	void HandleAccentKey(UWordTileWidget* Key);

	UFUNCTION()
	void ShowHomeFromButton() { ShowHome(); }

	UPROPERTY()
	TObjectPtr<UVerticalBox> Body;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> CodeBox;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> TypeBox;

	UPROPERTY()
	TObjectPtr<UWrapBox> AnswerRow;

	UPROPERTY()
	TObjectPtr<UWrapBox> TileBank;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusLine;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Language;

	UPROPERTY()
	TArray<TObjectPtr<ULanguagePackAsset>> Languages;

	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, int32> ChoiceOptions;

	UPROPERTY()
	TArray<TObjectPtr<UWordTileWidget>> AnswerTiles;

	FName RunningPhase;
};
