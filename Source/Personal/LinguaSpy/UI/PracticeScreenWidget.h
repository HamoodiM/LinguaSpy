// LinguaSpy - "Safe house" overlay: spaced-repetition review outside missions

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "PracticeScreenWidget.generated.h"

class ULanguagePackAsset;
class UConversationWidget;
class UConversationSubsystem;

/**
 *  Shows per-skill strength and how many phrases are due, and starts a review session.
 *  The review itself runs in a UConversationWidget on top (the same tile UI as missions); this panel hides meanwhile.
 */
UCLASS()
class UPracticeScreenWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Practice")
	void SetPack(ULanguagePackAsset* InPack);

	virtual void Close() override;

	/** Items per review session. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Practice")
	int32 SessionSize = 10;

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	void Refresh();
	UConversationSubsystem* GetConversations() const;

	UFUNCTION()
	void HandleStart();

	UFUNCTION()
	void HandleReviewFinished(int32 Passed, int32 Total);

	UFUNCTION()
	void HandleConversationEnded();

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Pack;

	UPROPERTY()
	TObjectPtr<UConversationWidget> DrillWidget;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> SkillBox;

	UPROPERTY()
	TObjectPtr<UButton> StartButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> StartLabel;
};
