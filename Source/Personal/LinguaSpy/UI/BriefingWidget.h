// LinguaSpy - mission briefing: meet the mission's phrases before you have to say them

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "MissionAsset.h"
#include "BriefingWidget.generated.h"

class ULanguagePackAsset;
class UMissionAsset;
class UVerticalBox;
class UWordTileWidget;

/**
 *  Comprehensible input before output (docs/Learning_Design.md section 1):
 *  1. Study: one card per phrase (text, reading, meaning, situation, audio).
 *  2. Quiz (no cover at stake): hear or read a phrase -> pick its meaning; then situation -> pick the phrase.
 *     A missed item comes back once at the end.
 *  Closing it marks the briefing done (UMissionSubsystem::MarkBriefingCompleted) and logs the first-try score.
 */
UCLASS()
class UBriefingWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	void SetMission(UMissionAsset* InMission, ULanguagePackAsset* InLanguage, bool bInReviewOnly = false);

	virtual void Close() override;

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual float GetPanelWidth() const override { return 820.f; }

private:

	enum class EQuizKind : uint8 { Listen, Read, Situation };

	struct FQuizItem
	{
		int32 Chunk = 0;
		EQuizKind Kind = EQuizKind::Read;
		bool bRetry = false;
	};

	void ShowStudyCard();
	void ShowQuizItem();
	void ShowSummary();
	void BuildQuiz();
	void ClearBody();
	void PlayChunk(int32 Index) const;
	UWordTileWidget* AddOption(const FString& Label, bool bNative);

	UFUNCTION()
	void HandleNext();

	UFUNCTION()
	void HandleHear();

	UFUNCTION()
	void HandleOption(UWordTileWidget* Option);

	UPROPERTY()
	TObjectPtr<UMissionAsset> Mission;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Language;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Body;

	UPROPERTY()
	TObjectPtr<UTextBlock> StepText;

	UPROPERTY()
	TObjectPtr<UButton> HearButton;

	UPROPERTY()
	TObjectPtr<UButton> NextButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> NextLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> FeedbackLine;

	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, int32> Options;

	TArray<FQuizItem> Quiz;
	int32 StudyIndex = 0;
	int32 QuizIndex = INDEX_NONE;
	int32 CorrectOption = INDEX_NONE;
	int32 FirstTryCorrect = 0;
	int32 FirstTryTotal = 0;
	int32 HearChunk = INDEX_NONE;
	bool bAnswered = false;
	bool bDone = false;
	bool bReviewOnly = false;
};
