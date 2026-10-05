// LinguaSpy - phrasebook overlay: phrases the player has learned in a city

#include "PhrasebookWidget.h"
#include "LinguaSpySaveSubsystem.h"
#include "LanguagePackAsset.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "WordTileWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpyPhrasebook"

FText UPhrasebookWidget::GetTitle() const
{
	return LOCTEXT("Title", "PHRASEBOOK");
}

void UPhrasebookWidget::BuildContent()
{
	using namespace LinguaSpyStyle;

	SummaryText = MakeText(FText::GetEmpty(), 17, TextMuted);
	AddRow(ContentBox, SummaryText, 0.f);

	FilterBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("FilterBox"));
	FilterBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	AddRow(ContentBox, FilterBox, 12.f);

	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ListSize->SetHeightOverride(560.f);
	List = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("List"));
	ListSize->SetContent(List);
	AddRow(ContentBox, ListSize, 14.f);
}

void UPhrasebookWidget::SetPack(ULanguagePackAsset* InPack)
{
	Pack = InPack;
	ActiveFilter = NAME_None;
	Items.Reset();
	const USpacedRepetitionSubsystem* Srs = GetGameInstance() ? GetGameInstance()->GetSubsystem<USpacedRepetitionSubsystem>() : nullptr;
	if (Pack && Srs)
	{
		Items = Srs->GetLearnedItems(Pack->CityId);
		SetTitle(FText::Format(LOCTEXT("TitleCity", "PHRASEBOOK - {0}"), Pack->CityName));
	}
	RebuildFilters();
	RebuildList();
}

const FName UPhrasebookWidget::FieldNotesFilter(TEXT("__FieldNotes"));

TArray<FGrammarNote> UPhrasebookWidget::GetSeenNotes() const
{
	TArray<FGrammarNote> Out;
	const ULinguaSpySaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
	if (Pack && Save && Save->GetSave())
	{
		for (const FGrammarNote& Note : Pack->GrammarNotes)
		{
			if (Save->GetSave()->SeenFieldNotes.Contains(Note.NoteId))
			{
				Out.Add(Note);
			}
		}
	}
	return Out;
}

void UPhrasebookWidget::RebuildFilters()
{
	FilterButtons.Reset();
	if (!FilterBox)
	{
		return;
	}
	FilterBox->ClearChildren();

	TArray<FName> Skills;
	for (const FSrsItemView& Item : Items)
	{
		Skills.AddUnique(Item.State.SkillTag);
	}
	const bool bHasNotes = !GetSeenNotes().IsEmpty();
	if (Skills.Num() < 2 && !bHasNotes)
	{
		return; // nothing to filter
	}
	Skills.Sort([](FName A, FName B) { return A.LexicalLess(B); });
	Skills.Insert(NAME_None, 0);
	if (bHasNotes)
	{
		Skills.Add(FieldNotesFilter);
	}

	for (FName Skill : Skills)
	{
		UWordTileWidget* Chip = CreateWidget<UWordTileWidget>(this, UWordTileWidget::StaticClass());
		Chip->SetWord(Skill.IsNone() ? LOCTEXT("All", "All").ToString()
			: Skill == FieldNotesFilter ? LOCTEXT("FieldNotes", "Field notes").ToString() : Skill.ToString());
		Chip->SetChoiceStyle();
		Chip->SetTileState(Skill == ActiveFilter ? EWordTileState::Correct : EWordTileState::Neutral);
		Chip->OnTileClicked.AddDynamic(this, &UPhrasebookWidget::HandleFilterClicked);
		FilterButtons.Add(Chip, Skill);
		FilterBox->AddChildToWrapBox(Chip);
	}
}

void UPhrasebookWidget::HandleFilterClicked(UWordTileWidget* Chip)
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	ActiveFilter = FilterButtons.FindRef(Chip);
	for (const TPair<TObjectPtr<UWordTileWidget>, FName>& Pair : FilterButtons)
	{
		Pair.Key->SetTileState(Pair.Value == ActiveFilter ? EWordTileState::Correct : EWordTileState::Neutral);
	}
	RebuildList();
}

void UPhrasebookWidget::RebuildList()
{
	if (!List)
	{
		return;
	}
	List->ClearChildren();

	if (ActiveFilter == FieldNotesFilter)
	{
		for (const FGrammarNote& Note : GetSeenNotes())
		{
			UVerticalBox* Entry = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			AddRow(Entry, MakeText(Note.Title, 20, LinguaSpyStyle::TextHelper, true), 0.f);
			AddRow(Entry, MakeText(Note.Body, 16, LinguaSpyStyle::TextMain), 4.f);
			if (UScrollBoxSlot* EntrySlot = Cast<UScrollBoxSlot>(List->AddChild(Entry)))
			{
				EntrySlot->SetPadding(FMargin(0.f, 0.f, 12.f, 16.f));
			}
		}
		return;
	}

	int32 Shown = 0;
	int32 Due = 0;
	for (const FSrsItemView& Item : Items)
	{
		Due += Item.bDue ? 1 : 0;
		if (!ActiveFilter.IsNone() && Item.State.SkillTag != ActiveFilter)
		{
			continue;
		}
		if (UScrollBoxSlot* EntrySlot = Cast<UScrollBoxSlot>(List->AddChild(MakeEntry(Item))))
		{
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 12.f, 10.f));
		}
		++Shown;
	}

	if (SummaryText)
	{
		SummaryText->SetText(Items.IsEmpty()
			? LOCTEXT("Empty", "No phrases yet. Answer locals correctly on missions and they'll be collected here.")
			: FText::Format(LOCTEXT("Summary", "{0} phrases learned - {1} due for review in the Safe house."), Items.Num(), Due));
	}
	if (Shown == 0 && !Items.IsEmpty() && SummaryText)
	{
		List->AddChild(MakeText(LOCTEXT("NoneInFilter", "Nothing in this skill yet."), 17, LinguaSpyStyle::TextMuted));
	}
}

UWidget* UPhrasebookWidget::MakeEntry(const FSrsItemView& Item)
{
	using namespace LinguaSpyStyle;

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	StyleBorder(Card, PanelInset, 10.f);
	Card->SetPadding(FMargin(18.f, 12.f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Phrase = MakeText(FText::FromString(Item.Phrase), 24, TextMain, true);
	if (Pack && !Pack->NativeFont.IsNull())
	{
		if (UFont* Font = Pack->NativeFont.LoadSynchronous())
		{
			FSlateFontInfo Info = Phrase->GetFont();
			Info.FontObject = Font;
			Info.TypefaceFontName = NAME_None;
			Phrase->SetFont(Info);
		}
	}
	Texts->AddChildToVerticalBox(Phrase);
	if (!Item.Reading.IsEmpty())
	{
		AddRow(Texts, MakeText(FText::FromString(Item.Reading), 16, TextMuted), 2.f);
	}
	AddRow(Texts, MakeText(Item.Meaning, 17, TextHelper), 4.f);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Texts))
	{
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Skill + strength (+ due tag)
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Skill = MakeText(FText::FromName(Item.State.SkillTag), 15, Accent, true);
	Skill->SetJustification(ETextJustify::Right);
	Side->AddChildToVerticalBox(Skill);
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarSize->SetWidthOverride(130.f);
	BarSize->SetHeightOverride(10.f);
	BarSize->SetContent(MakeBar(USrsScheduler::GetStrength(Item.State), TileCorrect, 10.f));
	AddRow(Side, BarSize, 6.f);
	if (Item.bDue)
	{
		UTextBlock* DueText = MakeText(LOCTEXT("DueTag", "due for review"), 14, TileWrong, true);
		DueText->SetJustification(ETextJustify::Right);
		AddRow(Side, DueText, 4.f);
	}
	if (UHorizontalBoxSlot* SideSlot = Row->AddChildToHorizontalBox(Side))
	{
		SideSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
		SideSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Card;
}

#undef LOCTEXT_NAMESPACE
