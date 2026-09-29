// LinguaSpy - a single clickable word tile

#include "WordTileWidget.h"
#include "LinguaSpyStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"

void UWordTileWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Default layout when no Blueprint layout was provided
	if (!WidgetTree->RootWidget)
	{
		TileButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TileButton"));
		WidgetTree->RootWidget = TileButton;

		TileText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TileText"));
		LinguaSpyStyle::StyleText(TileText, 20, LinguaSpyStyle::TileText, true);
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(TileButton->AddChild(TileText)))
		{
			ButtonSlot->SetPadding(FMargin(14.f, 6.f));
		}
	}

	if (TileButton)
	{
		TileButton->OnClicked.AddDynamic(this, &UWordTileWidget::HandleClicked);
	}
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
