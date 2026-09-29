// LinguaSpy - conversation panel: NPC line, helper translation, word-tile answer builder

#include "ConversationWidget.h"
#include "WordTileWidget.h"
#include "LinguaSpyStyle.h"
#include "NpcCharacter.h"
#include "SentenceGrader.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "LinguaSpyConversation"

void UConversationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!TileClass)
	{
		TileClass = UWordTileWidget::StaticClass();
	}
	if (!WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}

	if (HelperToggleButton) { HelperToggleButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleHelperToggle); }
	if (ClearButton)        { ClearButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleClear); }
	if (SubmitButton)       { SubmitButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleSubmit); }
	if (ContinueButton)     { ContinueButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleContinue); }
	if (LeaveButton)        { LeaveButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleLeave); }

	SetVisibility(ESlateVisibility::Collapsed);
}

void UConversationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->OnConversationStarted.AddUniqueDynamic(this, &UConversationWidget::HandleConversationStarted);
		Conversations->OnConversationEnded.AddUniqueDynamic(this, &UConversationWidget::HandleConversationEnded);
		Conversations->OnNodeEntered.AddUniqueDynamic(this, &UConversationWidget::HandleNodeEntered);
		Conversations->OnAnswerGraded.AddUniqueDynamic(this, &UConversationWidget::HandleAnswerGraded);
		Conversations->OnCoverBlown.AddUniqueDynamic(this, &UConversationWidget::HandleCoverBlown);
	}
}

void UConversationWidget::NativeDestruct()
{
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->OnConversationStarted.RemoveAll(this);
		Conversations->OnConversationEnded.RemoveAll(this);
		Conversations->OnNodeEntered.RemoveAll(this);
		Conversations->OnAnswerGraded.RemoveAll(this);
		Conversations->OnCoverBlown.RemoveAll(this);
	}
	Super::NativeDestruct();
}

UConversationSubsystem* UConversationWidget::GetConversationSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UConversationSubsystem>() : nullptr;
}

UButton* UConversationWidget::MakeButton(const FText& Label, const FName& Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetBackgroundColor(LinguaSpyStyle::Accent);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	LinguaSpyStyle::StyleText(Text, 18, LinguaSpyStyle::TileText, true);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
	{
		ButtonSlot->SetPadding(FMargin(18.f, 8.f));
	}
	return Button;
}

void UConversationWidget::BuildDefaultLayout()
{
	using namespace LinguaSpyStyle;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Bottom panel spanning most of the screen width
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(PanelBg);
	Panel->SetPadding(FMargin(28.f, 20.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.12f, 1.f, 0.88f, 1.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetOffsets(FMargin(0.f, -24.f, 0.f, 0.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	auto AddRow = [Column](UWidget* Widget, float TopPadding)
	{
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
	};

	// Header: speaker name + mode banner
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerText"));
	StyleText(SpeakerText, 20, Accent, true);
	Header->AddChildToHorizontalBox(SpeakerText);
	USpacer* HeaderSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* SpacerSlot = Header->AddChildToHorizontalBox(HeaderSpacer))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	ModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeText"));
	StyleText(ModeText, 18, Danger, true);
	Header->AddChildToHorizontalBox(ModeText);
	AddRow(Header, 0.f);

	NativeLineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NativeLineText"));
	StyleText(NativeLineText, 30, TextMain, true);
	NativeLineText->SetAutoWrapText(true);
	AddRow(NativeLineText, 8.f);

	HelperText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelperText"));
	StyleText(HelperText, 19, TextHelper);
	HelperText->SetAutoWrapText(true);
	AddRow(HelperText, 4.f);

	// Challenge area
	UVerticalBox* Challenge = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChallengePanel"));
	ChallengePanel = Challenge;

	IntentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntentText"));
	StyleText(IntentText, 18, TextMuted);
	IntentText->SetAutoWrapText(true);
	Challenge->AddChildToVerticalBox(IntentText);

	UBorder* AnswerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AnswerBorder"));
	AnswerBorder->SetBrushColor(PanelInset);
	AnswerBorder->SetPadding(FMargin(10.f));
	USizeBox* AnswerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AnswerSize"));
	AnswerSize->SetMinDesiredHeight(52.f);
	AnswerRow = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("AnswerRow"));
	AnswerRow->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	AnswerSize->SetContent(AnswerRow);
	AnswerBorder->SetContent(AnswerSize);
	if (UVerticalBoxSlot* AnswerSlot = Challenge->AddChildToVerticalBox(AnswerBorder))
	{
		AnswerSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	TileBank = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("TileBank"));
	TileBank->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	if (UVerticalBoxSlot* BankSlot = Challenge->AddChildToVerticalBox(TileBank))
	{
		BankSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	AddRow(Challenge, 14.f);

	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackText"));
	StyleText(FeedbackText, 19, TextMain, true);
	FeedbackText->SetAutoWrapText(true);
	AddRow(FeedbackText, 12.f);

	// Buttons
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	auto AddButton = [Buttons](UButton* Button)
	{
		if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}
	};
	HelperToggleButton = MakeButton(LOCTEXT("HelperToggle", "Helper"), TEXT("HelperToggleButton"));
	AddButton(HelperToggleButton);
	LeaveButton = MakeButton(LOCTEXT("Leave", "Walk away"), TEXT("LeaveButton"));
	AddButton(LeaveButton);
	USpacer* ButtonSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* SpacerSlot = Buttons->AddChildToHorizontalBox(ButtonSpacer))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	ClearButton = MakeButton(LOCTEXT("Clear", "Clear"), TEXT("ClearButton"));
	AddButton(ClearButton);
	SubmitButton = MakeButton(LOCTEXT("Submit", "Say it"), TEXT("SubmitButton"));
	AddButton(SubmitButton);
	ContinueButton = MakeButton(LOCTEXT("Continue", "Continue"), TEXT("ContinueButton"));
	AddButton(ContinueButton);
	AddRow(Buttons, 16.f);
}

void UConversationWidget::HandleConversationStarted(ANpcCharacter* Npc)
{
	if (SpeakerText)
	{
		SpeakerText->SetText(Npc ? Npc->GetDisplayName() : FText::GetEmpty());
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UConversationWidget::HandleConversationEnded()
{
	SetVisibility(ESlateVisibility::Collapsed);
	AnswerTiles.Reset();
	BankTiles.Reset();
	if (AnswerRow) { AnswerRow->ClearChildren(); }
	if (TileBank) { TileBank->ClearChildren(); }
}

void UConversationWidget::HandleNodeEntered(const FConversationNode& Node, EConversationMode Mode)
{
	if (NativeLineText)
	{
		NativeLineText->SetText(Node.NativeLine);
	}
	CurrentHelperLine = Node.HelperTranslation;
	UpdateHelperVisibility();

	if (ModeText)
	{
		const UConversationSubsystem* Conversations = GetConversationSubsystem();
		ModeText->SetText(Mode == EConversationMode::Practice && Conversations
			? FText::Format(LOCTEXT("PracticeBanner", "PRACTICE DRILL - {0} left"), Conversations->GetPracticeRemaining())
			: FText::GetEmpty());
	}
	if (FeedbackText)
	{
		FeedbackText->SetText(FText::GetEmpty());
	}

	AnswerTiles.Reset();
	BankTiles.Reset();
	if (AnswerRow) { AnswerRow->ClearChildren(); }
	if (TileBank) { TileBank->ClearChildren(); }
	bTilesLocked = false;

	const bool bIsChallenge = Node.Type == EConversationNodeType::PlayerChallenge;
	if (ChallengePanel)
	{
		ChallengePanel->SetVisibility(bIsChallenge ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (bIsChallenge)
	{
		if (IntentText)
		{
			IntentText->SetText(FText::Format(LOCTEXT("IntentFormat", "Your reply: {0}"), Node.Challenge.IntentPrompt));
		}

		TArray<FString> Words = Node.Challenge.TileBank;
		for (int32 i = Words.Num() - 1; i > 0; --i)
		{
			Words.Swap(i, FMath::RandRange(0, i));
		}
		for (const FString& Word : Words)
		{
			UWordTileWidget* Tile = CreateWidget<UWordTileWidget>(this, TileClass);
			Tile->SetWord(Word);
			Tile->OnTileClicked.AddDynamic(this, &UConversationWidget::HandleTileClicked);
			BankTiles.Add(Tile);
			if (TileBank) { TileBank->AddChildToWrapBox(Tile); }
		}
	}

	SetButtons(bIsChallenge, !bIsChallenge);
}

void UConversationWidget::HandleTileClicked(UWordTileWidget* Tile)
{
	if (bTilesLocked || !Tile)
	{
		return;
	}

	Tile->RemoveFromParent();
	if (Tile->bInAnswer)
	{
		AnswerTiles.Remove(Tile);
		BankTiles.Add(Tile);
		if (TileBank) { TileBank->AddChildToWrapBox(Tile); }
	}
	else
	{
		BankTiles.Remove(Tile);
		AnswerTiles.Add(Tile);
		if (AnswerRow) { AnswerRow->AddChildToWrapBox(Tile); }
	}
	Tile->bInAnswer = !Tile->bInAnswer;
}

void UConversationWidget::HandleClear()
{
	if (bTilesLocked)
	{
		return;
	}
	while (AnswerTiles.Num() > 0)
	{
		HandleTileClicked(AnswerTiles[0]);
	}
}

void UConversationWidget::HandleSubmit()
{
	if (bTilesLocked)
	{
		return;
	}
	if (AnswerTiles.IsEmpty())
	{
		if (FeedbackText)
		{
			FeedbackText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::TextMuted));
			FeedbackText->SetText(LOCTEXT("EmptyAnswer", "Build a sentence from the tiles first."));
		}
		return;
	}

	TArray<FString> Tokens;
	for (const UWordTileWidget* Tile : AnswerTiles)
	{
		Tokens.Add(Tile->GetWord());
	}
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->SubmitAnswer(Tokens);
	}
}

void UConversationWidget::HandleAnswerGraded(const FGradeResult& Result, const FString& ModelAnswer)
{
	bTilesLocked = true;
	for (int32 i = 0; i < AnswerTiles.Num(); ++i)
	{
		const bool bCorrect = Result.TileCorrect.IsValidIndex(i) && Result.TileCorrect[i];
		AnswerTiles[i]->SetTileState(bCorrect ? EWordTileState::Correct : EWordTileState::Wrong);
	}

	if (FeedbackText)
	{
		const UConversationSubsystem* Conversations = GetConversationSubsystem();
		const bool bPractice = Conversations && Conversations->GetMode() == EConversationMode::Practice;
		if (Result.bPassed)
		{
			FeedbackText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::TileCorrect));
			FeedbackText->SetText(bPractice
				? LOCTEXT("PracticePass", "Parfait ! That one's locked in.")
				: LOCTEXT("Pass", "Parfait ! Your cover holds."));
		}
		else
		{
			FeedbackText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::TileWrong));
			FeedbackText->SetText(FText::Format(bPractice
				? LOCTEXT("PracticeFail", "Not yet - it will come back later in the drill. Correct: \"{0}\"")
				: LOCTEXT("Fail", "The local narrows their eyes... Correct: \"{0}\""),
				FText::FromString(ModelAnswer)));
		}
	}

	SetButtons(false, true);
}

void UConversationWidget::HandleCoverBlown()
{
	if (ModeText)
	{
		ModeText->SetText(LOCTEXT("CoverBlownBanner", "COVER BLOWN"));
	}
	if (FeedbackText)
	{
		FeedbackText->SetColorAndOpacity(FSlateColor(LinguaSpyStyle::Danger));
		FeedbackText->SetText(LOCTEXT("CoverBlown", "Your cover is blown! Your handler pulls you out - time to drill the phrases that gave you away."));
	}
	SetButtons(false, true);
}

void UConversationWidget::HandleHelperToggle()
{
	bHelperHidden = !bHelperHidden;
	UpdateHelperVisibility();
}

void UConversationWidget::UpdateHelperVisibility()
{
	if (HelperText)
	{
		HelperText->SetText(bHelperHidden
			? LOCTEXT("HelperHidden", "(Helper muted - click Helper to hear the translation)")
			: FText::Format(LOCTEXT("HelperFormat", "Helper: \"{0}\""), CurrentHelperLine));
	}
}

void UConversationWidget::HandleContinue()
{
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->Continue();
	}
}

void UConversationWidget::HandleLeave()
{
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->EndConversation();
	}
}

void UConversationWidget::SetButtons(bool bShowSubmit, bool bShowContinue)
{
	const ESlateVisibility SubmitVis = bShowSubmit ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (SubmitButton) { SubmitButton->SetVisibility(SubmitVis); }
	if (ClearButton) { ClearButton->SetVisibility(SubmitVis); }
	if (ContinueButton) { ContinueButton->SetVisibility(bShowContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
}

#undef LOCTEXT_NAMESPACE
