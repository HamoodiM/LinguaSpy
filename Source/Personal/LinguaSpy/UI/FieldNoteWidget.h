// LinguaSpy - field note card: a short grammar note once the player has met a pattern a few times

#pragma once

#include "CoreMinimal.h"
#include "OverlayPanelWidget.h"
#include "LanguagePackAsset.h"
#include "FieldNoteWidget.generated.h"

/** Shows one FGrammarNote (noticing + brief explicit instruction: docs/Learning_Design.md section 4). */
UCLASS()
class UFieldNoteWidget : public UOverlayPanelWidget
{
	GENERATED_BODY()

public:

	void SetNote(const FGrammarNote& Note);

protected:

	virtual void BuildContent() override;
	virtual FText GetTitle() const override;
	virtual float GetPanelWidth() const override { return 640.f; }

private:

	UPROPERTY()
	TObjectPtr<UTextBlock> NoteTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> NoteBody;
};
