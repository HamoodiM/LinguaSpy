// LinguaSpy - a single clickable word tile

#include "WordTileWidget.h"
#include "LinguaSpyStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UWordTileWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Default layout when no Blueprint layout was provided
	if (!WidgetTree->RootWidget)
	{
		TileButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TileButton"));
		WidgetTree->RootWidget = TileButton;

		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(TileButton->AddChild(Stack)))
		{
			ButtonSlot->SetPadding(FMargin(14.f, 6.f));
		}

		TileText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TileText"));
		LinguaSpyStyle::StyleText(TileText, 20, LinguaSpyStyle::TileText, true);
		TileText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TextSlot = Stack->AddChildToVerticalBox(TileText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
		}

		ReadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReadingText"));
		LinguaSpyStyle::StyleText(ReadingText, 12, LinguaSpyStyle::TileText);
		ReadingText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* ReadingSlot = Stack->AddChildToVerticalBox(ReadingText))
		{
			ReadingSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	if (TileButton)
	{
		TileButton->OnClicked.AddDynamic(this, &UWordTileWidget::HandleClicked);
	}
	SetReading(FString());
	SetTileState(EWordTileState::Neutral);
}

void UWordTileWidget::SetWord(const FString& InWord)
{
	Word = InWord;
	if (TileText)
	{
		TileText->SetText(FText::FromString(Word));
	}
}

void UWordTileWidget::SetReading(const FString& InReading)
{
	if (ReadingText)
	{
		ReadingText->SetText(FText::FromString(InReading));
		ReadingText->SetVisibility(InReading.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UWordTileWidget::SetChoiceStyle()
{
	if (TileText)
	{
		LinguaSpyStyle::StyleText(TileText, 17, LinguaSpyStyle::TileText, true);
	}
}

void UWordTileWidget::SetTileState(EWordTileState NewState)
{
	if (!TileButton)
	{
		return;
	}
	switch (NewState)
	{
	case EWordTileState::Correct: TileButton->SetBackgroundColor(LinguaSpyStyle::TileCorrect); break;
	case EWordTileState::Wrong:   TileButton->SetBackgroundColor(LinguaSpyStyle::TileWrong); break;
	default:                      TileButton->SetBackgroundColor(LinguaSpyStyle::TileNeutral); break;
	}
}

void UWordTileWidget::SetLocked(bool bLocked)
{
	if (TileButton)
	{
		TileButton->SetIsEnabled(!bLocked);
	}
}

void UWordTileWidget::HandleClicked()
{
	OnTileClicked.Broadcast(this);
}
