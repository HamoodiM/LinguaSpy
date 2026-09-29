// LinguaSpy - one city on the city-select screen

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CityCardWidget.generated.h"

class UButton;
class UTextBlock;
class ULanguagePackAsset;
class UCityCardWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityCardClicked, ULanguagePackAsset*, Pack);

/**
 *  Clickable card: city, language, tagline, mission progress, best grades, review badge and weakest skill.
 *  Below it: Safe house (review) and Phrasebook buttons. Builds a default layout in C++.
 */
UCLASS()
class UCityCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Play the city. */
	UPROPERTY(BlueprintAssignable, Category="City")
	FOnCityCardClicked OnCardClicked;

	UPROPERTY(BlueprintAssignable, Category="City")
	FOnCityCardClicked OnPracticeClicked;

	UPROPERTY(BlueprintAssignable, Category="City")
	FOnCityCardClicked OnPhrasebookClicked;

	void SetPack(ULanguagePackAsset* InPack, int32 CompletedMissions, int32 TotalMissions);

protected:

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CityText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TaglineText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** "5 phrases to review" (hidden when nothing is due). */
	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DueText;

	/** "Work on: Directions" (weakest skill, hidden if none). */
	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> WeakSkillText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PracticeButton;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PhrasebookButton;

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandlePractice();

	UFUNCTION()
	void HandlePhrasebook();

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Pack;
};
