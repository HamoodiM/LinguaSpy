// LinguaSpy - phrasebook overlay: phrases the player has learned in a city

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "SpacedRepetitionSubsystem.h"
#include "PhrasebookWidget.generated.h"

class UScrollBox;
class UWrapBox;
class ULanguagePackAsset;
class UWordTileWidget;

/** Lists every phrase passed at least once in a city (phrase, reading, meaning, skill, strength), filterable by skill. */
UCLASS()
class UPhrasebookWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Phrasebook")
	void SetPack(ULanguagePackAsset* InPack);

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual float GetPanelWidth() const override { return 860.f; }

private:

	void RebuildFilters();
	void RebuildList();

	/** Field notes (grammar) the player has unlocked in this pack's language. */
	TArray<struct FGrammarNote> GetSeenNotes() const;

	/** Pseudo skill filter that lists field notes instead of phrases. */
	static const FName FieldNotesFilter;
	UWidget* MakeEntry(const FSrsItemView& Item);

	UFUNCTION()
	void HandleFilterClicked(UWordTileWidget* Chip);

	UPROPERTY()
	TObjectPtr<UWrapBox> FilterBox;

	UPROPERTY()
	TObjectPtr<UScrollBox> List;

	UPROPERTY()
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Pack;

	/** Filter chip -> skill (None = all). */
	UPROPERTY()
	TMap<TObjectPtr<UWordTileWidget>, FName> FilterButtons;

	TArray<FSrsItemView> Items;
	FName ActiveFilter;
};
