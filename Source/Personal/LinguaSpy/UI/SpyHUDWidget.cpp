// LinguaSpy - always-on HUD: cover meter and hints

#include "SpyHUDWidget.h"
#include "CoverComponent.h"
#include "LinguaSpyStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LinguaSpyHUD"

void USpyHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USpyHUDWidget::BuildDefaultLayout()
{
	using namespace LinguaSpyStyle;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CoverPanel"));
	Panel->SetBrushColor(PanelBg);
	Panel->SetPadding(FMargin(16.f, 12.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(24.f, 24.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);

	CoverLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CoverLabel"));
	StyleText(CoverLabel, 16, Accent, true);
	CoverLabel->SetText(LOCTEXT("CoverLabel", "COVER"));
	Column->AddChildToVerticalBox(CoverLabel);

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarSize->SetWidthOverride(260.f);
	BarSize->SetHeightOverride(18.f);
	CoverBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CoverBar"));
	CoverBar->SetPercent(1.f);
	CoverBar->SetFillColorAndOpacity(TileCorrect);
	BarSize->SetContent(CoverBar);
	if (UVerticalBoxSlot* BarSlot = Column->AddChildToVerticalBox(BarSize))
	{
		BarSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	StyleText(HintText, 15, TextMuted);
	HintText->SetText(LOCTEXT("DefaultHint", "Click a local to talk to them."));
	if (UVerticalBoxSlot* HintSlot = Column->AddChildToVerticalBox(HintText))
	{
		HintSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
}

void USpyHUDWidget::BindCover(UCoverComponent* Cover)
{
	if (UCoverComponent* Old = BoundCover.Get())
	{
		Old->OnCoverChanged.RemoveAll(this);
	}
	BoundCover = Cover;
	if (Cover)
	{
		Cover->OnCoverChanged.AddUniqueDynamic(this, &USpyHUDWidget::HandleCoverChanged);
		HandleCoverChanged(Cover->GetCover(), Cover->GetMaxCover());
	}
}

void USpyHUDWidget::SetHint(const FText& Hint)
{
	if (HintText)
	{
		HintText->SetText(Hint);
	}
}

void USpyHUDWidget::HandleCoverChanged(float NewCover, float MaxCover)
{
	if (!CoverBar)
	{
		return;
	}
	const float Percent = MaxCover > 0.f ? NewCover / MaxCover : 0.f;
	CoverBar->SetPercent(Percent);
	const FLinearColor Fill = Percent > 0.6f ? LinguaSpyStyle::TileCorrect
		: Percent > 0.3f ? LinguaSpyStyle::Accent
		: LinguaSpyStyle::Danger;
	CoverBar->SetFillColorAndOpacity(Fill);
}

void USpyHUDWidget::NativeDestruct()
{
	BindCover(nullptr);
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
