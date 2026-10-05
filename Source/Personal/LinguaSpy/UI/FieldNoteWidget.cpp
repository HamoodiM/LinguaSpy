// LinguaSpy - field note card: a short grammar note once the player has met a pattern a few times

#include "FieldNoteWidget.h"
#include "LinguaSpyStyle.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "LinguaSpyFieldNote"

FText UFieldNoteWidget::GetTitle() const
{
	return LOCTEXT("Title", "FIELD NOTE");
}

void UFieldNoteWidget::BuildContent()
{
	using namespace LinguaSpyStyle;
	AddRow(ContentBox, MakeText(LOCTEXT("Intro", "Your handler noticed a pattern in what you've been saying:"), 16, TextMuted), 0.f);
	NoteTitle = MakeText(FText::GetEmpty(), 24, TextHelper, true);
	AddRow(ContentBox, NoteTitle, 12.f);
	NoteBody = MakeText(FText::GetEmpty(), 19, TextMain);
	AddRow(ContentBox, NoteBody, 8.f);
	AddRow(ContentBox, MakeText(LOCTEXT("Saved", "Saved to your phrasebook under Field notes."), 14, TextMuted), 16.f);
}

void UFieldNoteWidget::SetNote(const FGrammarNote& Note)
{
	if (NoteTitle) { NoteTitle->SetText(Note.Title); }
	if (NoteBody) { NoteBody->SetText(Note.Body); }
}

#undef LOCTEXT_NAMESPACE
