// LinguaSpy - assessment overlay: participant code, pre/post/delayed tests, placement (no feedback per item)

#include "AssessmentWidget.h"
#include "AssessmentSubsystem.h"
#include "LanguagePackAsset.h"
#include "LearningTelemetrySubsystem.h"
#include "LinguaSpyAudio.h"
#include "LinguaSpySaveSubsystem.h"
#include "LinguaSpyStyle.h"
#include "MissionSubsystem.h"
#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"
#include "WordTileWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LinguaSpyAssessment"

FText UAssessmentWidget::GetTitle() const
{
	return LOCTEXT("Title", "FIELD TEST");
}

UAssessmentSubsystem* UAssessmentWidget::GetAssessment() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UAssessmentSubsystem>() : nullptr;
}

bool UAssessmentWidget::IsPilot() const
{
	const ULearningTelemetrySubsystem* Telemetry = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULearningTelemetrySubsystem>() : nullptr;
	return Telemetry && Telemetry->IsPilotMode();
}

void UAssessmentWidget::BuildContent()
{
	Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddRow(ContentBox, Body, 0.f);
	StatusLine = MakeText(FText::GetEmpty(), 15, LinguaSpyStyle::TextMuted);
	AddRow(ContentBox, StatusLine, 14.f);

	// Languages that have test forms
	UMissionSubsystem* Missions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Missions && GetAssessment())
	{
		for (ULanguagePackAsset* Pack : Missions->GetAllLanguagePacks())
		{
			if (!GetAssessment()->GetForms(Pack).IsEmpty())
			{
				Languages.Add(Pack);
			}
		}
	}
	Language = Languages.IsEmpty() ? nullptr : Languages[0].Get();

	const ULinguaSpySaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
	if (IsPilot() && Save && Save->GetParticipantCode().IsEmpty())
	{
		ShowCodeEntry();
	}
	else
	{
		ShowHome();
	}
}

void UAssessmentWidget::ClearBody()
{
	ChoiceOptions.Reset();
	AnswerTiles.Reset();
	TypeBox = nullptr;
	AnswerRow = nullptr;
	TileBank = nullptr;
	if (Body)
	{
		Body->ClearChildren();
	}
}

UButton* UAssessmentWidget::AddButtonRow(const FText& Label)
{
	UButton* Button = MakeButton(Label);
	AddRow(Body, Button, 10.f);
	return Button;
}

void UAssessmentWidget::ShowCodeEntry()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	AddRow(Body, MakeText(LOCTEXT("CodeTitle", "Participant code"), 24, Accent, true), 0.f);
	AddRow(Body, MakeText(LOCTEXT("CodeHelp",
		"Enter the code your facilitator gave you (for example MT-07). Please don't type your name. "
		"The game keeps a log of your answers on this computer only, under that code."), 16, TextMain), 8.f);
	CodeBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	CodeBox->SetHintText(LOCTEXT("CodeHint", "MT-07"));
	AddRow(Body, CodeBox, 12.f);
	UButton* SaveButton = AddButtonRow(LOCTEXT("CodeSave", "Save code"));
	SaveButton->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleCodeSave);
}

void UAssessmentWidget::HandleCodeSave()
{
	ULinguaSpySaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULinguaSpySaveSubsystem>() : nullptr;
	if (!Save || !CodeBox)
	{
		return;
	}
	if (!Save->SetParticipantCode(CodeBox->GetText().ToString()))
	{
		StatusLine->SetText(LOCTEXT("CodeBad", "Codes use letters, digits and '-', up to 8 characters (e.g. MT-07)."));
		return;
	}
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	StatusLine->SetText(FText::Format(LOCTEXT("CodeOk", "Participant {0}."), FText::FromString(Save->GetParticipantCode())));
	ShowHome();
}

void UAssessmentWidget::ShowHome()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	UAssessmentSubsystem* Assessment = GetAssessment();
	if (!Language || !Assessment)
	{
		AddRow(Body, MakeText(LOCTEXT("NoForms", "No test forms are installed."), 18, TextMain), 0.f);
		return;
	}

	AddRow(Body, MakeText(FText::Format(LOCTEXT("Lang", "Language: {0} ({1})"), Language->LanguageName, Language->CityName), 22, Accent, true), 0.f);
	if (Languages.Num() > 1)
	{
		UButton* Switch = AddButtonRow(LOCTEXT("Switch", "Switch language"));
		Switch->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleLanguage);
	}

	auto PhaseLine = [this, Assessment](FName Phase, const FText& Label) -> FText
	{
		FAssessmentResult Result;
		if (Assessment->GetLatestResult(Language, Phase, Result))
		{
			const FText When = FText::AsDate(FDateTime(Result.UtcTicks), EDateTimeStyle::Medium);
			return IsPilot()
				? FText::Format(LOCTEXT("DonePilot", "{0} - done ({1})"), Label, When)
				: FText::Format(LOCTEXT("DoneScore", "{0} - {1} / {2} ({3})"), Label, Result.Score, Result.MaxScore, When);
		}
		return Label;
	};

	AddRow(Body, MakeText(LOCTEXT("NoFeedback",
		"About 15 short questions, 5-8 minutes. You won't see whether each answer is right - just do your best, and guess if unsure."),
		16, TextMain), 10.f);

	if (IsPilot())
	{
		UButton* Pre = AddButtonRow(PhaseLine(LinguaSpyAssessmentPhase::Pre, LOCTEXT("Pre", "1. Before playing")));
		Pre->OnClicked.AddDynamic(this, &UAssessmentWidget::HandlePre);
		UButton* Post = AddButtonRow(PhaseLine(LinguaSpyAssessmentPhase::Post, LOCTEXT("Post", "2. After playing")));
		Post->OnClicked.AddDynamic(this, &UAssessmentWidget::HandlePost);
		UButton* Delayed = AddButtonRow(PhaseLine(LinguaSpyAssessmentPhase::Delayed, LOCTEXT("Delayed", "3. One week later")));
		Delayed->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleDelayed);
		UButton* Logs = AddButtonRow(LOCTEXT("Logs", "Facilitator: open the log folder"));
		Logs->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleOpenLogs);
	}
	else
	{
		UButton* Placement = AddButtonRow(PhaseLine(LinguaSpyAssessmentPhase::Placement, LOCTEXT("Placement", "Check my level")));
		Placement->OnClicked.AddDynamic(this, &UAssessmentWidget::HandlePlacement);
		AddRow(Body, MakeText(LOCTEXT("PlacementHelp",
			"Already know some? A good score tells your handler to step back sooner in this city."), 15, TextMuted), 6.f);
	}
}

void UAssessmentWidget::HandleLanguage()
{
	const int32 Index = Languages.IndexOfByKey(Language);
	Language = Languages[(Index + 1) % Languages.Num()];
	ShowHome();
}

void UAssessmentWidget::HandlePre()       { StartPhase(LinguaSpyAssessmentPhase::Pre); }
void UAssessmentWidget::HandlePost()      { StartPhase(LinguaSpyAssessmentPhase::Post); }
void UAssessmentWidget::HandleDelayed()   { StartPhase(LinguaSpyAssessmentPhase::Delayed); }
void UAssessmentWidget::HandlePlacement() { StartPhase(LinguaSpyAssessmentPhase::Placement); }

void UAssessmentWidget::HandleOpenLogs()
{
	if (const ULearningTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<ULearningTelemetrySubsystem>())
	{
		Telemetry->OpenLogFolder();
		StatusLine->SetText(FText::FromString(Telemetry->GetLogDirectory()));
	}
}

void UAssessmentWidget::StartPhase(FName Phase)
{
	LinguaSpyAudio::PlayUI(this, ESpyUISound::MenuSelect);
	UAssessmentSubsystem* Assessment = GetAssessment();
	UAssessmentAsset* Form = Assessment ? Assessment->ChooseForm(Language, Phase) : nullptr;
	if (!Form || !Assessment->StartRun(Form, Phase))
	{
		StatusLine->SetText(LOCTEXT("NoStart", "Couldn't start the test."));
		return;
	}
	RunningPhase = Phase;
	ShowItem();
}

void UAssessmentWidget::ShowItem()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	UAssessmentSubsystem* Assessment = GetAssessment();
	const FAssessmentItem* Item = Assessment ? Assessment->GetCurrentItem() : nullptr;
	if (!Item)
	{
		ShowDone();
		return;
	}
	int32 Index = 0;
	int32 Count = 0;
	Assessment->GetProgress(Index, Count);
	StatusLine->SetText(FText::Format(LOCTEXT("Progress", "Question {0} of {1}"), Index + 1, Count));

	AddRow(Body, MakeText(Item->Prompt, 22, TextMain, true), 0.f);
	const bool bSpaces = !Language || Language->bUsesWordSpacing;

	switch (Item->Type)
	{
	case EAssessmentItemType::ListenChoose:
		if (!Item->Audio.IsNull())
		{
			UButton* Hear = AddButtonRow(LOCTEXT("Hear", "Play again"));
			Hear->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleHear);
			LinguaSpyAudio::PlayVoice(this, Item->Audio);
		}
		else
		{
			AddRow(Body, MakeText(FText::FromString(Item->TargetText), 32, Accent, true), 8.f);
		}
		break;
	case EAssessmentItemType::MeaningChoose:
		AddRow(Body, MakeText(FText::FromString(Item->TargetText), 32, Accent, true), 8.f);
		break;
	case EAssessmentItemType::GapFill:
		AddRow(Body, MakeText(FText::FromString(UScaffoldPolicy::BuildGapDisplay(Item->Challenge, bSpaces)), 30, Accent, true), 8.f);
		break;
	case EAssessmentItemType::Assemble:
		break;
	}

	if (Item->IsChoice())
	{
		for (int32 i = 0; i < Item->Options.Num(); ++i)
		{
			UWordTileWidget* Option = CreateWidget<UWordTileWidget>(GetOwningPlayer(), UWordTileWidget::StaticClass());
			Option->SetWord(Item->Options[i].ToString());
			Option->SetChoiceStyle();
			Option->OnTileClicked.AddDynamic(this, &UAssessmentWidget::HandleChoice);
			ChoiceOptions.Add(Option, i);
			AddRow(Body, Option, 8.f);
		}
		return;
	}

	if (Item->Type == EAssessmentItemType::GapFill)
	{
		TypeBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		TypeBox->SetHintText(LOCTEXT("TypeGap", "Type the missing word(s)"));
		AddRow(Body, TypeBox, 12.f);
		TypeBox->SetKeyboardFocus();
	}
	else
	{
		AnswerRow = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
		AnswerRow->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		AddRow(Body, MakeText(LOCTEXT("YourSentence", "Your sentence:"), 15, TextMuted), 10.f);
		AddRow(Body, AnswerRow, 4.f);
		TileBank = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
		TileBank->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		AddRow(Body, TileBank, 14.f);
		TArray<FWordTile> Tiles = Item->Challenge.Tiles;
		for (int32 i = Tiles.Num() - 1; i > 0; --i)
		{
			Tiles.Swap(i, FMath::RandRange(0, i));
		}
		for (const FWordTile& WordTile : Tiles)
		{
			UWordTileWidget* Tile = CreateWidget<UWordTileWidget>(GetOwningPlayer(), UWordTileWidget::StaticClass());
			Tile->SetWord(WordTile.Text);
			Tile->OnTileClicked.AddDynamic(this, &UAssessmentWidget::HandleTile);
			TileBank->AddChildToWrapBox(Tile);
		}
	}
	UButton* Next = AddButtonRow(LOCTEXT("Next", "Next"));
	Next->OnClicked.AddDynamic(this, &UAssessmentWidget::HandleNext);
}

void UAssessmentWidget::HandleHear()
{
	if (const FAssessmentItem* Item = GetAssessment() ? GetAssessment()->GetCurrentItem() : nullptr)
	{
		LinguaSpyAudio::PlayVoice(this, Item->Audio);
	}
}

void UAssessmentWidget::HandleChoice(UWordTileWidget* Option)
{
	const int32* Index = ChoiceOptions.Find(Option);
	if (UAssessmentSubsystem* Assessment = Index ? GetAssessment() : nullptr)
	{
		LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
		Assessment->SubmitChoice(*Index);
		ShowItem();
	}
}

void UAssessmentWidget::HandleTile(UWordTileWidget* Tile)
{
	if (!Tile || !AnswerRow || !TileBank)
	{
		return;
	}
	Tile->RemoveFromParent();
	if (Tile->bInAnswer)
	{
		AnswerTiles.Remove(Tile);
		TileBank->AddChildToWrapBox(Tile);
	}
	else
	{
		AnswerTiles.Add(Tile);
		AnswerRow->AddChildToWrapBox(Tile);
	}
	Tile->bInAnswer = !Tile->bInAnswer;
	LinguaSpyAudio::PlayUI(this, ESpyUISound::TilePlace);
}

void UAssessmentWidget::HandleNext()
{
	UAssessmentSubsystem* Assessment = GetAssessment();
	if (!Assessment)
	{
		return;
	}
	LinguaSpyAudio::PlayUI(this, ESpyUISound::Click);
	TArray<FString> Tokens;
	if (TypeBox)
	{
		Tokens = USentenceGrader::TokenizeTyped(TypeBox->GetText().ToString());
	}
	else
	{
		for (const UWordTileWidget* Tile : AnswerTiles)
		{
			Tokens.Add(Tile->GetWord());
		}
	}
	Assessment->SubmitTokens(Tokens);
	ShowItem();
}

void UAssessmentWidget::ShowDone()
{
	using namespace LinguaSpyStyle;
	ClearBody();
	LinguaSpyAudio::StopVoice(this);
	const FAssessmentResult Result = GetAssessment() ? GetAssessment()->GetLastResult() : FAssessmentResult();
	StatusLine->SetText(FText::GetEmpty());
	AddRow(Body, MakeText(LOCTEXT("Thanks", "Done - thank you!"), 28, Accent, true), 0.f);
	if (IsPilot())
	{
		AddRow(Body, MakeText(LOCTEXT("SavedPilot", "Your answers are saved. Let your facilitator know you're finished."), 18, TextMain), 8.f);
	}
	else
	{
		AddRow(Body, MakeText(FText::Format(LOCTEXT("Score", "You got {0} of {1}."), Result.Score, Result.MaxScore), 22, TextMain, true), 8.f);
		if (RunningPhase == LinguaSpyAssessmentPhase::Placement)
		{
			const bool bOnCall = Result.MaxScore > 0 && static_cast<float>(Result.Score) / Result.MaxScore >= UAssessmentSubsystem::PlacementOnCallThreshold;
			AddRow(Body, MakeText(bOnCall
				? LOCTEXT("PlacementHigh", "Nice! Your handler will only translate when you ask (On call).")
				: LOCTEXT("PlacementLow", "Your handler will translate every line at first (Shadowing) and step back as you improve."),
				17, TextHelper), 8.f);
		}
	}
	UButton* Back = AddButtonRow(LOCTEXT("Back", "Back"));
	Back->OnClicked.AddDynamic(this, &UAssessmentWidget::ShowHomeFromButton);
}

void UAssessmentWidget::Close()
{
	if (UAssessmentSubsystem* Assessment = GetAssessment())
	{
		if (Assessment->IsRunning())
		{
			Assessment->CancelRun();
		}
	}
	LinguaSpyAudio::StopVoice(this);
	Super::Close();
}

#undef LOCTEXT_NAMESPACE
