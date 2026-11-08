// LinguaSpy - base for full-screen overlay panels (settings, phrasebook, safe house)

#include "OverlayPanelWidget.h"
#include "LinguaSpyStyle.h"
#include "LinguaSpyAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LinguaSpyOverlay"

void UOverlayPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree->RootWidget)
	{
		using namespace LinguaSpyStyle;

		Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
		StyleBorder(Backdrop, FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f);
		Backdrop->SetHorizontalAlignment(HAlign_Center);
		Backdrop->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Backdrop;

		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
		Size->SetWidthOverride(GetPanelWidth());
		Size->SetMaxDesiredHeight(860.f);
		Backdrop->SetContent(Size);

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		StyleBorder(Panel, PanelBg, PanelRadius);
		Panel->SetPadding(FMargin(34.f, 26.f));
		Size->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		Panel->SetContent(Column);

		// Header: title + close
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		TitleText = MakeText(GetTitle(), 30, Accent, true);
		if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}
		CloseButton = MakeButton(LOCTEXT("Close", "Close"));
		Header->AddChildToHorizontalBox(CloseButton);
		Column->AddChildToVerticalBox(Header);

		ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
		if (UVerticalBoxSlot* ContentSlot = Column->AddChildToVerticalBox(ContentBox))
		{
			ContentSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UOverlayPanelWidget::HandleCloseClicked);
	}
	if (ContentBox)
	{
		BuildContent();
	}
}

void UOverlayPanelWidget::Open()
{
	AddToViewport(120);
	SetIsFocusable(true); // without it SetKeyboardFocus is refused ("does not support focus" warning)
	SetKeyboardFocus();
}

void UOverlayPanelWidget::Close()
{
	RemoveFromParent();
	OnClosed.Broadcast();
}

void UOverlayPanelWidget::HandleCloseClicked()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	Close();
}

void UOverlayPanelWidget::SetTitle(const FText& Title)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
}

UTextBlock* UOverlayPanelWidget::MakeText(const FText& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LinguaSpyStyle::StyleText(Block, Size, Color, bBold);
	Block->SetText(Text);
	Block->SetAutoWrapText(true);
	return Block;
}

UButton* UOverlayPanelWidget::MakeButton(const FText& Label, UTextBlock** OutLabel, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	LinguaSpyStyle::StyleButton(Button, LinguaSpyStyle::ButtonRadius);
	Button->SetBackgroundColor(LinguaSpyStyle::Accent);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LinguaSpyStyle::StyleText(Text, FontSize, LinguaSpyStyle::TileText, true);
	Text->SetText(Label);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
	{
		ButtonSlot->SetPadding(FMargin(18.f, 8.f));
	}
	if (OutLabel)
	{
		*OutLabel = Text;
	}
	return Button;
}

UProgressBar* UOverlayPanelWidget::MakeBar(float Percent, const FLinearColor& Fill, float Height)
{
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	FProgressBarStyle Style;
	Style.SetBackgroundImage(LinguaSpyStyle::RoundedBrush(LinguaSpyStyle::PanelInset, Height * 0.5f));
	Style.SetFillImage(LinguaSpyStyle::RoundedBrush(FLinearColor::White, Height * 0.5f));
	Style.SetMarqueeImage(LinguaSpyStyle::RoundedBrush(FLinearColor::White, Height * 0.5f));
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(Fill);
	Bar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
	return Bar;
}

void UOverlayPanelWidget::AddRow(UVerticalBox* Box, UWidget* Widget, float TopPadding)
{
	if (Box && Widget)
	{
		if (UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
	}
}

#undef LOCTEXT_NAMESPACE
