// LinguaSpy - always-on HUD: cover meter, objective tracker, mission banners

#include "SpyHUDWidget.h"
#include "CoverComponent.h"
#include "LinguaSpyStyle.h"
#include "MissionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpyHUD"

void USpyHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	if (BannerPanel)
	{
		BannerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USpyHUDWidget::BuildDefaultLayout()
{
	using namespace LinguaSpyStyle;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top-left: cover meter + objective
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CoverPanel"));
	Panel->SetBrushColor(PanelBg);
	Panel->SetPadding(FMargin(16.f, 12.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(24.f, 24.f));
	}

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Width->SetWidthOverride(320.f);
	Panel->SetContent(Width);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Width->SetContent(Column);

	auto AddRow = [Column](UWidget* Widget, float TopPadding)
	{
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
	};

	CoverLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CoverLabel"));
	StyleText(CoverLabel, 16, Accent, true);
	CoverLabel->SetText(LOCTEXT("CoverLabel", "COVER"));
	AddRow(CoverLabel, 0.f);

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarSize->SetHeightOverride(18.f);
	CoverBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CoverBar"));
	CoverBar->SetPercent(1.f);
	CoverBar->SetFillColorAndOpacity(TileCorrect);
	BarSize->SetContent(CoverBar);
	AddRow(BarSize, 6.f);

	MissionTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MissionTitleText"));
	StyleText(MissionTitleText, 15, Accent, true);
	MissionTitleText->SetAutoWrapText(true);
	AddRow(MissionTitleText, 12.f);

	ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObjectiveText"));
	StyleText(ObjectiveText, 16, TextMain);
	ObjectiveText->SetAutoWrapText(true);
	AddRow(ObjectiveText, 2.f);

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	StyleText(HintText, 14, TextMuted);
	HintText->SetAutoWrapText(true);
	HintText->SetText(LOCTEXT("DefaultHint", "Click a local to talk to them. P: pause."));
	AddRow(HintText, 8.f);

	// Top-centre banner
	BannerPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BannerPanel"));
	BannerPanel->SetBrushColor(PanelBg);
	BannerPanel->SetPadding(FMargin(40.f, 18.f));
	if (UCanvasPanelSlot* BannerSlot = Root->AddChildToCanvas(BannerPanel))
	{
		BannerSlot->SetAnchors(FAnchors(0.5f, 0.f));
		BannerSlot->SetAlignment(FVector2D(0.5f, 0.f));
		BannerSlot->SetAutoSize(true);
		BannerSlot->SetPosition(FVector2D(0.f, 90.f));
	}
	UVerticalBox* BannerColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BannerPanel->SetContent(BannerColumn);
	BannerTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BannerTitle"));
	StyleText(BannerTitle, 30, Accent, true);
	BannerTitle->SetJustification(ETextJustify::Center);
	BannerColumn->AddChildToVerticalBox(BannerTitle);
	BannerSubtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BannerSubtitle"));
	StyleText(BannerSubtitle, 17, TextMain);
	BannerSubtitle->SetJustification(ETextJustify::Center);
	BannerSubtitle->SetAutoWrapText(true);
	BannerColumn->AddChildToVerticalBox(BannerSubtitle);
}

UMissionSubsystem* USpyHUDWidget::GetMissionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMissionSubsystem>() : nullptr;
}

void USpyHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UMissionSubsystem* Missions = GetMissionSubsystem())
	{
		Missions->OnObjectiveChanged.AddUniqueDynamic(this, &USpyHUDWidget::HandleObjectiveChanged);
		Missions->OnMissionStarted.AddUniqueDynamic(this, &USpyHUDWidget::HandleMissionStarted);
		Missions->OnMissionCompleted.AddUniqueDynamic(this, &USpyHUDWidget::HandleMissionCompleted);
		Missions->OnMissionFailed.AddUniqueDynamic(this, &USpyHUDWidget::HandleMissionFailed);
		Missions->OnCityCompleted.AddUniqueDynamic(this, &USpyHUDWidget::HandleCityCompleted);

		// The city may already be running (widget created after the mission started)
		if (UMissionAsset* Mission = Missions->GetActiveMission())
		{
			HandleMissionStarted(Mission);
		}
	}
	HandleObjectiveChanged();
}

void USpyHUDWidget::NativeDestruct()
{
	BindCover(nullptr);
	if (UMissionSubsystem* Missions = GetMissionSubsystem())
	{
		Missions->OnObjectiveChanged.RemoveAll(this);
		Missions->OnMissionStarted.RemoveAll(this);
		Missions->OnMissionCompleted.RemoveAll(this);
		Missions->OnMissionFailed.RemoveAll(this);
		Missions->OnCityCompleted.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void USpyHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (BannerTimeLeft > 0.f)
	{
		BannerTimeLeft -= InDeltaTime;
		if (BannerTimeLeft <= 0.f && BannerPanel)
		{
			BannerPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
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

void USpyHUDWidget::ShowBanner(const FText& Title, const FText& Subtitle, float Duration)
{
	if (BannerTitle) { BannerTitle->SetText(Title); }
	if (BannerSubtitle)
	{
		BannerSubtitle->SetText(Subtitle);
		BannerSubtitle->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (BannerPanel)
	{
		BannerPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	BannerTimeLeft = Duration;
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

void USpyHUDWidget::HandleObjectiveChanged()
{
	const UMissionSubsystem* Missions = GetMissionSubsystem();
	const FText Objective = Missions ? Missions->GetCurrentObjectiveText() : FText::GetEmpty();
	if (ObjectiveText)
	{
		ObjectiveText->SetText(Objective.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("ObjectiveFormat", "> {0}"), Objective));
		ObjectiveText->SetVisibility(Objective.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (MissionTitleText)
	{
		const UMissionAsset* Mission = Missions ? Missions->GetActiveMission() : nullptr;
		MissionTitleText->SetText(Mission ? Mission->Title : FText::GetEmpty());
		MissionTitleText->SetVisibility(Mission ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void USpyHUDWidget::HandleMissionStarted(UMissionAsset* Mission)
{
	if (Mission)
	{
		ShowBanner(Mission->Title, Mission->Briefing, 6.f);
	}
	HandleObjectiveChanged();
}

void USpyHUDWidget::HandleMissionCompleted(UMissionAsset* Mission)
{
	ShowBanner(LOCTEXT("MissionComplete", "MISSION COMPLETE"), Mission ? Mission->Title : FText::GetEmpty(), 5.f);
}

void USpyHUDWidget::HandleMissionFailed(UMissionAsset* Mission)
{
	ShowBanner(LOCTEXT("MissionFailed", "COVER BURNED"), LOCTEXT("MissionFailedSub", "Your handler resets the operation. Try again."), 5.f);
}

void USpyHUDWidget::HandleCityCompleted()
{
	ShowBanner(LOCTEXT("CityComplete", "CITY CLEARED"), LOCTEXT("CityCompleteSub", "Every mission here is done. Press P to return to the menu."), 6.f);
}

#undef LOCTEXT_NAMESPACE
