// LinguaSpy - one city on the city-select screen

#include "CityCardWidget.h"
#include "LanguagePackAsset.h"
#include "LinguaSpyStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LinguaSpyCityCard"

void UCityCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree->RootWidget)
	{
		using namespace LinguaSpyStyle;

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardSize"));
		CardSize->SetWidthOverride(260.f);
		CardSize->SetHeightOverride(300.f);
		WidgetTree->RootWidget = CardSize;

		CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CardButton"));
		LinguaSpyStyle::StyleButton(CardButton, LinguaSpyStyle::CardRadius);
		CardSize->SetContent(CardButton);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(CardButton->AddChild(Column)))
		{
			ButtonSlot->SetPadding(FMargin(20.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}

		auto AddText = [this, Column](const TCHAR* Name, int32 FontSize, bool bBold, float TopPadding) -> UTextBlock*
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
			StyleText(Text, FontSize, LinguaSpyStyle::TileText, bBold);
			Text->SetAutoWrapText(true);
			if (UVerticalBoxSlot* TextSlot = Column->AddChildToVerticalBox(Text))
			{
				TextSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
			}
			return Text;
		};
		CityText = AddText(TEXT("CityText"), 32, true, 0.f);
		LanguageText = AddText(TEXT("LanguageText"), 18, true, 2.f);
		TaglineText = AddText(TEXT("TaglineText"), 15, false, 16.f);
		ProgressText = AddText(TEXT("ProgressText"), 15, true, 16.f);
	}

	if (CardButton)
	{
		CardButton->OnClicked.AddDynamic(this, &UCityCardWidget::HandleClicked);
	}
}

void UCityCardWidget::SetPack(ULanguagePackAsset* InPack, int32 CompletedMissions, int32 TotalMissions)
{
	Pack = InPack;
	if (!Pack)
	{
		return;
	}
	const bool bPlayable = !Pack->CityMap.IsNull();

	if (CityText) { CityText->SetText(Pack->CityName); }
	if (LanguageText) { LanguageText->SetText(Pack->LanguageName); }
	if (TaglineText) { TaglineText->SetText(Pack->Tagline); }
	if (ProgressText)
	{
		ProgressText->SetText(!bPlayable ? LOCTEXT("ComingSoon", "Coming soon")
			: TotalMissions == 0 ? LOCTEXT("NoMissions", "Free roam")
			: FText::Format(LOCTEXT("Progress", "Missions {0}/{1}"), CompletedMissions, TotalMissions));
	}
	if (CardButton)
	{
		CardButton->SetBackgroundColor(Pack->AccentColor);
		CardButton->SetIsEnabled(bPlayable);
	}
}

void UCityCardWidget::HandleClicked()
{
	if (Pack)
	{
		OnCardClicked.Broadcast(Pack);
	}
}

#undef LOCTEXT_NAMESPACE
