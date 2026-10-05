// LinguaSpy - conversation panel: NPC line, helper translation, choices, scaffolded answer input, intel checks

#include "ConversationWidget.h"
#include "WordTileWidget.h"
#include "LinguaSpyStyle.h"
#include "LanguagePackAsset.h"
#include "LinguaSpySaveSubsystem.h"
#include "NpcCharacter.h"
#include "SentenceGrader.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LinguaSpyAudio.h"

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

	if (HelperToggleButton)  { HelperToggleButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleHelperToggle); }
	if (ReadingToggleButton) { ReadingToggleButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleReadingToggle); }
	if (RepeatButton)        { RepeatButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleRepeat); }
	if (SlowButton)          { SlowButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleSlow); }
	if (ClearButton)         { ClearButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleClear); }
	if (SubmitButton)        { SubmitButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleSubmit); }
	if (ContinueButton)      { ContinueButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleContinue); }
	if (LeaveButton)         { LeaveButton->OnClicked.AddDynamic(this, &UConversationWidget::HandleLeave); }
	if (TypeBox)             { TypeBox->OnTextCommitted.AddDynamic(this, &UConversationWidget::HandleTypeCommitted); }

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
		Conversations->OnChallengeReady.AddUniqueDynamic(this, &UConversationWidget::HandleChallengeReady);
		Conversations->OnAnswerFeedback.AddUniqueDynamic(this, &UConversationWidget::HandleAnswerFeedback);
		Conversations->OnIntelAnswered.AddUniqueDynamic(this, &UConversationWidget::HandleIntelAnswered);
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
		Conversations->OnChallengeReady.RemoveAll(this);
		Conversations->OnAnswerFeedback.RemoveAll(this);
		Conversations->OnIntelAnswered.RemoveAll(this);
		Conversations->OnCoverBlown.RemoveAll(this);
	}
	Super::NativeDestruct();
}

UConversationSubsystem* UConversationWidget::GetConversationSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UConversationSubsystem>() : nullptr;
}

ULinguaSpySaveSubsystem* UConversationWidget::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
}

bool UConversationWidget::IsHelperEnabled() const
{
	const UConversationSubsystem* Conversations = GetConversationSubsystem();
	return Conversations && Conversations->IsTranslationRevealed();
}

void UConversationWidget::SetButtonLabel(UButton* Button, const FText& Label) const
{
	if (UTextBlock* Text = Button ? Cast<UTextBlock>(Button->GetChildAt(0)) : nullptr)
	{
		Text->SetText(Label);
	}
}

bool UConversationWidget::ShouldShowReadings() const
{
	const ULinguaSpySaveSubsystem* Save = GetSaveSubsystem();
	return ActiveLanguage && ActiveLanguage->bHasReadings && (!Save || Save->GetSettings().bShowReadings);
}

bool UConversationWidget::IsTypingLevel() const
{
	return CurrentPresentation.Level == EScaffoldLevel::GapFill || CurrentPresentation.Level == EScaffoldLevel::Produce;
}

UButton* UConversationWidget::MakeButton(const FText& Label, const FName& Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	LinguaSpyStyle::StyleButton(Button, LinguaSpyStyle::ButtonRadius);
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
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel = PanelBorder;
	LinguaSpyStyle::StyleBorder(PanelBorder, PanelBg, LinguaSpyStyle::PanelRadius);
	PanelBorder->SetPadding(FMargin(28.f, 20.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.12f, 1.f, 0.88f, 1.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetOffsets(FMargin(0.f, -24.f, 0.f, 0.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	PanelBorder->SetContent(Column);

	auto AddRowTo = [](UVerticalBox* Box, UWidget* Widget, float TopPadding)
	{
		if (UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Widget))
		{
			RowSlot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
	};
	auto AddRow = [Column, &AddRowTo](UWidget* Widget, float TopPadding) { AddRowTo(Column, Widget, TopPadding); };

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

	ReadingLineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReadingLineText"));
	StyleText(ReadingLineText, 17, TextMuted);
	ReadingLineText->SetAutoWrapText(true);
	AddRow(ReadingLineText, 2.f);

	HelperText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelperText"));
	StyleText(HelperText, 19, TextHelper);
	HelperText->SetAutoWrapText(true);
	AddRow(HelperText, 4.f);

	ChoiceBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("ChoiceBox"));
	ChoiceBox->SetInnerSlotPadding(FVector2D(10.f, 10.f));
	AddRow(ChoiceBox, 14.f);

	// Challenge area
	UVerticalBox* Challenge = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChallengePanel"));
	ChallengePanel = Challenge;

	IntentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntentText"));
	StyleText(IntentText, 18, TextMain);
	IntentText->SetAutoWrapText(true);
	Challenge->AddChildToVerticalBox(IntentText);

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	StyleText(LevelText, 14, TextMuted);
	AddRowTo(Challenge, LevelText, 2.f);

	GapText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GapText"));
	StyleText(GapText, 26, TextMain, true);
	GapText->SetAutoWrapText(true);
	AddRowTo(Challenge, GapText, 10.f);

	OptionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionBox"));
	AddRowTo(Challenge, OptionBox, 10.f);

	UVerticalBox* Answer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AnswerArea"));
	AnswerArea = Answer;
	UBorder* AnswerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AnswerBorder"));
	LinguaSpyStyle::StyleBorder(AnswerBorder, PanelInset, 8.f);
	AnswerBorder->SetPadding(FMargin(10.f));
	USizeBox* AnswerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AnswerSize"));
	AnswerSize->SetMinDesiredHeight(52.f);
	AnswerRow = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("AnswerRow"));
	AnswerRow->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	AnswerSize->SetContent(AnswerRow);
	AnswerBorder->SetContent(AnswerSize);
	AddRowTo(Answer, AnswerBorder, 0.f);
	TileBank = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("TileBank"));
	TileBank->SetInnerSlotPadding(FVector2D(8.f, 8.f));
	AddRowTo(Answer, TileBank, 12.f);
	AddRowTo(Challenge, Answer, 10.f);

	TypeBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("TypeBox"));
	TypeBox->SetHintText(LOCTEXT("TypeHint", "Type here, then press Enter"));
	AddRowTo(Challenge, TypeBox, 10.f);

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
	HelperToggleButton = MakeButton(LOCTEXT("HelperToggle", "Translate"), TEXT("HelperToggleButton"));
	AddButton(HelperToggleButton);
	RepeatButton = MakeButton(LOCTEXT("Repeat", "Repeat"), TEXT("RepeatButton"));
	AddButton(RepeatButton);
	SlowButton = MakeButton(LOCTEXT("Slow", "Slower"), TEXT("SlowButton"));
	AddButton(SlowButton);
	ReadingToggleButton = MakeButton(LOCTEXT("ReadingToggle", "Romaji"), TEXT("ReadingToggleButton"));
	AddButton(ReadingToggleButton);
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

void UConversationWidget::ApplyNativeFont(UTextBlock* Text, int32 Size) const
{
	if (!Text || !ActiveLanguage || ActiveLanguage->NativeFont.IsNull())
	{
		return;
	}
	if (UFont* Font = ActiveLanguage->NativeFont.LoadSynchronous())
	{
		FSlateFontInfo Info = Text->GetFont();
		Info.FontObject = Font;
		Info.TypefaceFontName = NAME_None;
		Info.Size = Size;
		Text->SetFont(Info);
	}
}

void UConversationWidget::ShowFeedback(const FText& Text, const FLinearColor& Color)
{
	if (FeedbackText)
	{
		FeedbackText->SetColorAndOpacity(FSlateColor(Color));
		FeedbackText->SetText(Text);
	}
}

void UConversationWidget::HandleConversationStarted(ANpcCharacter* Npc)
{
	const UConversationSubsystem* Conversations = GetConversationSubsystem();
	ActiveLanguage = Conversations ? Conversations->GetActiveLanguage() : nullptr;

	const bool bReview = Conversations && Conversations->GetMode() == EConversationMode::Review;
	if (SpeakerText)
	{
		SpeakerText->SetText(Npc ? Npc->GetDisplayName()
			: bReview && Conversations->IsDebrief() ? LOCTEXT("DebriefSpeaker", "Debrief with your handler")
			: bReview ? LOCTEXT("ReviewSpeaker", "Safe house drill") : FText::GetEmpty());
	}
	SetButtonLabel(LeaveButton, bReview ? LOCTEXT("EndSession", "End session") : LOCTEXT("Leave", "Walk away"));
	if (ReadingToggleButton)
	{
		ReadingToggleButton->SetVisibility(ActiveLanguage && ActiveLanguage->bHasReadings ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	ApplyNativeFont(NativeLineText, 30);
	ApplyNativeFont(GapText, 26);
	if (SpeakerText && ActiveLanguage)
	{
		SpeakerText->SetColorAndOpacity(FSlateColor(ActiveLanguage->AccentColor));
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SlideTime = 0.f;
}

void UConversationWidget::HandleConversationEnded()
{
	SetVisibility(ESlateVisibility::Collapsed);
	ClearTiles();
	ClearChoices();
	ClearOptions();
	ActiveLanguage = nullptr;
}

void UConversationWidget::ClearTiles()
{
	AnswerTiles.Reset();
	BankTiles.Reset();
	if (AnswerRow) { AnswerRow->ClearChildren(); }
	if (TileBank) { TileBank->ClearChildren(); }
	if (TypeBox) { TypeBox->SetText(FText::GetEmpty()); }
	bTilesLocked = false;
}

void UConversationWidget::ClearChoices()
{
	ChoiceButtons.Reset();
	if (ChoiceBox)
	{
		ChoiceBox->ClearChildren();
		ChoiceBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UConversationWidget::ClearOptions()
{
	OptionButtons.Reset();
	if (OptionBox)
	{
		OptionBox->ClearChildren();
	}
}

UWordTileWidget* UConversationWidget::MakeOptionButton(const FString& Label, bool bNativeText)
{
	UWordTileWidget* Button = CreateWidget<UWordTileWidget>(this, TileClass);
	Button->SetWord(Label);
	Button->SetChoiceStyle();
	if (bNativeText && ActiveLanguage && !ActiveLanguage->NativeFont.IsNull())
	{
		Button->SetNativeFont(ActiveLanguage->NativeFont.LoadSynchronous());
	}
	Button->SetTileState(EWordTileState::Neutral);
	return Button;
}

void UConversationWidget::HandleNodeEntered(const FConversationNode& Node, EConversationMode Mode)
{
	if (NativeLineText)
	{
		NativeLineText->SetText(Node.NativeLine);
	}
	CurrentHelperLine = Node.HelperTranslation;
	CurrentReadingLine = Node.NativeLineReading;
	CurrentPresentation = FChallengePresentation();
	bIntelNode = Node.Type == EConversationNodeType::IntelCheck;
	RefreshHelperAndReadings();
	RefreshAudioButtons();

	if (ModeText)
	{
		const UConversationSubsystem* Conversations = GetConversationSubsystem();
		ModeText->SetText(Mode == EConversationMode::Practice && Conversations
			? FText::Format(LOCTEXT("PracticeBanner", "PRACTICE DRILL - {0} left"), Conversations->GetPracticeRemaining())
			: Mode == EConversationMode::Review && Conversations && Conversations->IsDebrief()
			? FText::Format(LOCTEXT("DebriefBanner", "DEBRIEF - {0} left"), Conversations->GetPracticeRemaining())
			: Mode == EConversationMode::Review && Conversations
			? FText::Format(LOCTEXT("ReviewBanner", "REVIEW - {0} left"), Conversations->GetPracticeRemaining())
			: FText::GetEmpty());
		ModeText->SetColorAndOpacity(FSlateColor(Mode == EConversationMode::Review ? LinguaSpyStyle::TextHelper : LinguaSpyStyle::Danger));
	}
	ShowFeedback(FText::GetEmpty(), LinguaSpyStyle::TextMain);

	ClearTiles();
	ClearChoices();
	ClearOptions();
	if (ChallengePanel)
	{
		ChallengePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Node.Type == EConversationNodeType::PlayerChoice)
	{
		const UConversationSubsystem* Conversations = GetConversationSubsystem();
		if (ChoiceBox)
		{
			ChoiceBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		for (int32 i = 0; i < Node.Choices.Num(); ++i)
		{
			if (Conversations && !Conversations->IsChoiceAvailable(i))
			{
				continue;
			}
			UWordTileWidget* Button = MakeOptionButton(Node.Choices[i].Label.ToString(), false);
			Button->OnTileClicked.AddDynamic(this, &UConversationWidget::HandleChoiceClicked);
			ChoiceButtons.Add(Button, i);
			if (ChoiceBox) { ChoiceBox->AddChildToWrapBox(Button); }
		}
		ShowFeedback(LOCTEXT("ChoosePrompt", "How do you respond?"), LinguaSpyStyle::TextMuted);
		SetButtons(false, false);
		return;
	}

	if (bIntelNode)
	{
		// Listening check: act on what the local said
		if (ChoiceBox)
		{
			ChoiceBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		for (int32 i = 0; i < Node.Intel.Options.Num(); ++i)
		{
			UWordTileWidget* Button = MakeOptionButton(Node.Intel.Options[i].ToString(), false);
			Button->OnTileClicked.AddDynamic(this, &UConversationWidget::HandleIntelClicked);
			OptionButtons.Add(Button, i);
			if (ChoiceBox) { ChoiceBox->AddChildToWrapBox(Button); }
		}
		ShowFeedback(FText::Format(LOCTEXT("IntelPrompt", "INTEL: {0}"), Node.Intel.Question), LinguaSpyStyle::Accent);
		SetButtons(false, false);
		return;
	}

	// PlayerChallenge nodes get their input from HandleChallengeReady (broadcast right after this)
	SetButtons(false, Node.Type != EConversationNodeType::PlayerChallenge);
}

void UConversationWidget::HandleChallengeReady(const FChallengePresentation& Presentation)
{
	CurrentPresentation = Presentation;
	const FWordTileChallenge& Challenge = Presentation.Challenge;
	ClearTiles();
	ClearChoices();
	ClearOptions();
	ShowFeedback(FText::GetEmpty(), LinguaSpyStyle::TextMain);
	if (ChallengePanel)
	{
		ChallengePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	const bool bRecognize = Presentation.Level == EScaffoldLevel::Recognize;
	const bool bTiles = Presentation.Level == EScaffoldLevel::Assemble || Presentation.Level == EScaffoldLevel::AssemblePlus;
	const bool bGap = Presentation.Level == EScaffoldLevel::GapFill;
	const bool bTyping = IsTypingLevel();

	if (IntentText)
	{
		IntentText->SetText(bRecognize
			? FText::Format(LOCTEXT("IntentRecognize", "Which would you say? {0}"), Challenge.IntentPrompt)
			: FText::Format(LOCTEXT("IntentFormat", "Your reply: {0}"), Challenge.IntentPrompt));
	}
	if (LevelText)
	{
		FText Level;
		switch (Presentation.Level)
		{
		case EScaffoldLevel::Recognize:    Level = LOCTEXT("LevelRecognize", "Handler support: pick the right sentence"); break;
		case EScaffoldLevel::Assemble:     Level = LOCTEXT("LevelAssemble", "Handler support: just the words you need"); break;
		case EScaffoldLevel::AssemblePlus: Level = LOCTEXT("LevelAssemblePlus", "Watch out: some words don't belong"); break;
		case EScaffoldLevel::GapFill:      Level = LOCTEXT("LevelGap", "From memory: type the missing word(s)"); break;
		case EScaffoldLevel::Produce:      Level = LOCTEXT("LevelProduce", "From memory: type the whole sentence"); break;
		}
		LevelText->SetText(Level);
	}
	if (GapText)
	{
		GapText->SetText(FText::FromString(Presentation.GapDisplay));
		GapText->SetVisibility(bGap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (AnswerArea)
	{
		AnswerArea->SetVisibility(bTiles ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TypeBox)
	{
		TypeBox->SetVisibility(bTyping ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		TypeBox->SetIsReadOnly(false);
		TypeBox->SetHintText(bGap
			? FText::Format(LOCTEXT("GapHint", "Type the {0} missing {0}|plural(one=word,other=words), then press Enter"), Presentation.GapCount)
			: LOCTEXT("ProduceHint", "Type your reply, then press Enter"));
		if (bTyping)
		{
			TypeBox->SetKeyboardFocus();
		}
	}

	if (bRecognize)
	{
		for (int32 i = 0; i < Presentation.RecognitionOptions.Num(); ++i)
		{
			UWordTileWidget* Button = MakeOptionButton(Presentation.RecognitionOptions[i], true);
			Button->OnTileClicked.AddDynamic(this, &UConversationWidget::HandleOptionClicked);
			OptionButtons.Add(Button, i);
			if (OptionBox)
			{
				if (UVerticalBoxSlot* OptionSlot = OptionBox->AddChildToVerticalBox(Button))
				{
					OptionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
					OptionSlot->SetHorizontalAlignment(HAlign_Left);
				}
			}
		}
	}

	if (bTiles)
	{
		TArray<FWordTile> Tiles = Presentation.Tiles;
		for (int32 i = Tiles.Num() - 1; i > 0; --i)
		{
			Tiles.Swap(i, FMath::RandRange(0, i));
		}
		const bool bReadings = ShouldShowReadings();
		UFont* NativeFont = ActiveLanguage && !ActiveLanguage->NativeFont.IsNull() ? ActiveLanguage->NativeFont.LoadSynchronous() : nullptr;
		for (const FWordTile& WordTile : Tiles)
		{
			UWordTileWidget* Tile = CreateWidget<UWordTileWidget>(this, TileClass);
			Tile->SetWord(WordTile.Text);
			Tile->SetNativeFont(NativeFont);
			Tile->SetReading(bReadings ? WordTile.Reading : FString());
			Tile->OnTileClicked.AddDynamic(this, &UConversationWidget::HandleTileClicked);
			BankTiles.Add(Tile);
			if (TileBank) { TileBank->AddChildToWrapBox(Tile); }
		}
	}

	SetButtons(!bRecognize, false);
	if (ClearButton)
	{
		ClearButton->SetVisibility(bTiles ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UConversationWidget::HandleChoiceClicked(UWordTileWidget* ChoiceButton)
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (const int32* Index = ChoiceButtons.Find(ChoiceButton))
	{
		if (UConversationSubsystem* Conversations = GetConversationSubsystem())
		{
			Conversations->SelectChoice(*Index);
		}
	}
}

void UConversationWidget::HandleOptionClicked(UWordTileWidget* OptionButton)
{
	const int32* Index = OptionButtons.Find(OptionButton);
	UConversationSubsystem* Conversations = GetConversationSubsystem();
	if (!Index || !Conversations || bTilesLocked)
	{
		return;
	}
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	const FAnswerFeedback Feedback = Conversations->SubmitRecognition(*Index);
	if (!Feedback.Grade.bPassed)
	{
		OptionButton->SetTileState(EWordTileState::Wrong);
		OptionButton->SetLocked(true);
		OptionButton->PlayShake();
	}
	else
	{
		OptionButton->SetTileState(EWordTileState::Correct);
	}
}

void UConversationWidget::HandleIntelClicked(UWordTileWidget* OptionButton)
{
	const int32* Index = OptionButtons.Find(OptionButton);
	if (UConversationSubsystem* Conversations = Index ? GetConversationSubsystem() : nullptr)
	{
		LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
		if (!Conversations->SubmitIntel(*Index))
		{
			OptionButton->SetTileState(EWordTileState::Wrong);
			OptionButton->SetLocked(true);
			OptionButton->PlayShake();
		}
		else
		{
			OptionButton->SetTileState(EWordTileState::Correct);
		}
	}
}

void UConversationWidget::HandleIntelAnswered(bool bCorrect, int32 ChoiceIndex)
{
	LinguaSpyAudio::PlayUI(this, bCorrect ? ESpyUISound::Pass : ESpyUISound::Fail);
	if (bCorrect)
	{
		for (const TPair<TObjectPtr<UWordTileWidget>, int32>& Pair : OptionButtons)
		{
			Pair.Key->SetLocked(true);
		}
		ShowFeedback(LOCTEXT("IntelRight", "Noted. That's the intel you needed."), LinguaSpyStyle::TileCorrect);
		SetButtons(false, true);
	}
	else
	{
		const UConversationSubsystem* Conversations = GetConversationSubsystem();
		ShowFeedback(Conversations && Conversations->HasLineAudio()
			? LOCTEXT("IntelWrongAudio", "That's not what they said. Listen again (Repeat / Slower) and try another answer.")
			: LOCTEXT("IntelWrong", "That's not what they said. Read the line again and try another answer."),
			LinguaSpyStyle::TileWrong);
	}
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
	Tile->SetTileState(EWordTileState::Neutral);
	Tile->PlayPop();
	LinguaSpyAudio::PlayUI(this, ESpyUISound::TilePlace);
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

void UConversationWidget::HandleTypeCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		HandleSubmit();
	}
}

void UConversationWidget::HandleSubmit()
{
	UConversationSubsystem* Conversations = GetConversationSubsystem();
	if (bTilesLocked || !Conversations)
	{
		return;
	}

	if (IsTypingLevel())
	{
		const TArray<FString> Words = USentenceGrader::TokenizeTyped(TypeBox ? TypeBox->GetText().ToString() : FString());
		if (Words.IsEmpty())
		{
			ShowFeedback(LOCTEXT("EmptyTyped", "Type your answer first."), LinguaSpyStyle::TextMuted);
			return;
		}
		if (CurrentPresentation.Level == EScaffoldLevel::GapFill)
		{
			Conversations->SubmitGapWords(Words);
		}
		else
		{
			Conversations->SubmitAnswer(Words);
		}
		return;
	}

	if (AnswerTiles.IsEmpty())
	{
		ShowFeedback(LOCTEXT("EmptyAnswer", "Build a sentence from the tiles first."), LinguaSpyStyle::TextMuted);
		return;
	}
	TArray<FString> Tokens;
	for (const UWordTileWidget* Tile : AnswerTiles)
	{
		Tokens.Add(Tile->GetWord());
	}
	Conversations->SubmitAnswer(Tokens);
}

void UConversationWidget::HandleAnswerFeedback(const FAnswerFeedback& Feedback)
{
	const FGradeResult& Result = Feedback.Grade;
	LinguaSpyAudio::PlayUI(this, Result.bPassed ? ESpyUISound::Pass : ESpyUISound::Fail);

	// Colour the placed tiles (prompt: right ones green, wrong ones red; the player fixes them and tries again)
	for (int32 i = 0; i < AnswerTiles.Num(); ++i)
	{
		const bool bCorrect = Result.TileCorrect.IsValidIndex(i) && Result.TileCorrect[i];
		AnswerTiles[i]->SetTileState(bCorrect ? EWordTileState::Correct : EWordTileState::Wrong);
		if (!bCorrect)
		{
			AnswerTiles[i]->PlayShake();
		}
	}

	if (!Feedback.bFinal)
	{
		// Feedback ladder step 1: a prompt, not the answer
		ShowFeedback(FText::Format(LOCTEXT("Prompt", "Handler: \"{0}\" Try again."), Feedback.Hint), LinguaSpyStyle::TextHelper);
		bTilesLocked = false;
		return;
	}

	bTilesLocked = true;
	if (TypeBox)
	{
		TypeBox->SetIsReadOnly(true);
	}
	for (const TPair<TObjectPtr<UWordTileWidget>, int32>& Pair : OptionButtons)
	{
		Pair.Key->SetLocked(true);
	}

	const UConversationSubsystem* Conversations = GetConversationSubsystem();
	const bool bPractice = Conversations
		&& (Conversations->GetMode() == EConversationMode::Practice || Conversations->GetMode() == EConversationMode::Review);
	if (Result.bPassed)
	{
		ShowFeedback(Feedback.bAssisted
			? FText::Format(LOCTEXT("AssistedPass", "Got it: \"{0}\". Next time, try it without help."), FText::FromString(Feedback.ModelAnswer))
			: bPractice ? LOCTEXT("PracticePass", "Perfect! That one's locked in.")
			: LOCTEXT("Pass", "Perfect! Your cover holds."),
			LinguaSpyStyle::TileCorrect);
	}
	else
	{
		// Feedback ladder step 2: the recast (model answer, spoken if recorded)
		ShowFeedback(FText::Format(bPractice
			? LOCTEXT("PracticeFail", "Here's how a local says it: \"{0}\". It will come back later in the drill.")
			: LOCTEXT("Fail", "The local narrows their eyes... A local would say: \"{0}\""),
			FText::FromString(Feedback.ModelAnswer)), LinguaSpyStyle::TileWrong);
	}

	SetButtons(false, true);
}

void UConversationWidget::HandleCoverBlown()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::CoverBlown);
	bTilesLocked = true;
	if (ModeText)
	{
		ModeText->SetText(LOCTEXT("CoverBlownBanner", "COVER BLOWN"));
	}
	ShowFeedback(LOCTEXT("CoverBlown", "Your cover is blown! Your handler pulls you out - time to drill the phrases that gave you away."),
		LinguaSpyStyle::Danger);
	SetButtons(false, true);
}

void UConversationWidget::HandleHelperToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		if (!Conversations->RevealTranslation() && Conversations->GetPeeksLeft() == 0)
		{
			ShowFeedback(LOCTEXT("RadioSilence", "Radio silence: your handler can't help any more in this conversation."), LinguaSpyStyle::TextMuted);
		}
	}
	RefreshHelperAndReadings();
}

void UConversationWidget::HandleReadingToggle()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (ULinguaSpySaveSubsystem* Save = GetSaveSubsystem())
	{
		Save->SetShowReadings(!Save->GetSettings().bShowReadings);
	}
	RefreshHelperAndReadings();

	// Update readings on tiles already shown
	const bool bReadings = ShouldShowReadings();
	for (UWordTileWidget* Tile : BankTiles)
	{
		Tile->SetReading(bReadings ? CurrentPresentation.Challenge.FindReading(Tile->GetWord()) : FString());
	}
	for (UWordTileWidget* Tile : AnswerTiles)
	{
		Tile->SetReading(bReadings ? CurrentPresentation.Challenge.FindReading(Tile->GetWord()) : FString());
	}
}

void UConversationWidget::HandleRepeat()
{
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->RepeatLine(false);
	}
}

void UConversationWidget::HandleSlow()
{
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->RepeatLine(true);
	}
}

void UConversationWidget::RefreshAudioButtons()
{
	const UConversationSubsystem* Conversations = GetConversationSubsystem();
	const ESlateVisibility Vis = Conversations && Conversations->HasLineAudio() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (RepeatButton) { RepeatButton->SetVisibility(Vis); }
	if (SlowButton) { SlowButton->SetVisibility(Vis); }
}

void UConversationWidget::RefreshHelperAndReadings()
{
	const bool bRevealed = IsHelperEnabled();
	const UConversationSubsystem* Conversations = GetConversationSubsystem();
	const int32 PeeksLeft = Conversations ? Conversations->GetPeeksLeft() : -1;
	if (HelperText)
	{
		HelperText->SetText(bRevealed
			? FText::Format(LOCTEXT("HelperFormat", "Handler: \"{0}\""), CurrentHelperLine)
			: PeeksLeft == 0 ? LOCTEXT("HelperSilent", "(Radio silence - you're on your own here.)")
			: PeeksLeft > 0 ? FText::Format(LOCTEXT("HelperLimited", "(Radio silence: {0} {0}|plural(one=translation,other=translations) left in this conversation.)"), PeeksLeft)
			: Conversations && Conversations->GetMode() != EConversationMode::Conversation
			? LOCTEXT("HelperHiddenPractice", "(Tap Translate if you need the line in English.)")
			: LOCTEXT("HelperHidden", "(Stuck? Tap Translate and your handler will whisper it - it lowers your fluency grade.)"));
		HelperText->SetColorAndOpacity(FSlateColor(bRevealed ? LinguaSpyStyle::TextHelper : LinguaSpyStyle::TextMuted));
	}
	if (HelperToggleButton)
	{
		HelperToggleButton->SetVisibility(bRevealed || CurrentHelperLine.IsEmpty() || PeeksLeft == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (ReadingLineText)
	{
		const bool bShow = ShouldShowReadings() && !CurrentReadingLine.IsEmpty();
		ReadingLineText->SetText(CurrentReadingLine);
		ReadingLineText->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UConversationWidget::HandleContinue()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	if (UConversationSubsystem* Conversations = GetConversationSubsystem())
	{
		Conversations->Continue();
	}
}

void UConversationWidget::HandleLeave()
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
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

void UConversationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Slide the panel up and fade it in when a conversation starts
	constexpr float SlideDuration = 0.25f;
	if (SlideTime >= 0.f && Panel)
	{
		SlideTime += InDeltaTime;
		const float Alpha = FMath::Clamp(SlideTime / SlideDuration, 0.f, 1.f);
		const float Eased = 1.f - FMath::Pow(1.f - Alpha, 3.f);
		Panel->SetRenderTranslation(FVector2D(0.f, (1.f - Eased) * 80.f));
		Panel->SetRenderOpacity(Eased);
		if (Alpha >= 1.f)
		{
			SlideTime = -1.f;
		}
	}
}

#undef LOCTEXT_NAMESPACE
