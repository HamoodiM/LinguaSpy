// LinguaSpy - automation tests for the Phase 5 learning loop: scaffolds, feedback, debrief, handler, assessment, telemetry

#include "Misc/AutomationTest.h"
#include "AssessmentAsset.h"
#include "AssessmentSubsystem.h"
#include "LearningTelemetrySubsystem.h"
#include "LinguaSpySaveGame.h"
#include "LinguaSpySaveSubsystem.h"
#include "MissionSubsystem.h"
#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace LinguaSpyPhase5Test
{
	// "je voudrais un café" + distractors "thé" "veux"
	FWordTileChallenge MakeCafe()
	{
		FWordTileChallenge Challenge;
		for (const TCHAR* Word : { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("thé"), TEXT("veux") })
		{
			FWordTile Tile;
			Tile.Text = Word;
			Challenge.Tiles.Add(Tile);
		}
		FAcceptedAnswer Answer;
		Answer.Tokens = { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café") };
		Challenge.AcceptedAnswers.Add(Answer);
		Challenge.SkillTag = TEXT("Ordering");
		Challenge.CoverPenalty = 20.f;
		return Challenge;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyScaffoldChooseTest, "LinguaSpy.Scaffold.Choose", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyScaffoldChooseTest::RunTest(const FString& Parameters)
{
	FSrsItemState State;
	TestEqual(TEXT("Never seen -> Assemble"), UScaffoldPolicy::ChooseLevel(nullptr, 0, true), EScaffoldLevel::Assemble);
	State.Box = 0;
	TestEqual(TEXT("Box 0 -> Assemble"), UScaffoldPolicy::ChooseLevel(&State, 0, true), EScaffoldLevel::Assemble);
	State.Box = 2;
	TestEqual(TEXT("Box 2 -> AssemblePlus"), UScaffoldPolicy::ChooseLevel(&State, 0, true), EScaffoldLevel::AssemblePlus);
	State.Box = 3;
	TestEqual(TEXT("Box 3 -> GapFill"), UScaffoldPolicy::ChooseLevel(&State, 0, true), EScaffoldLevel::GapFill);
	TestEqual(TEXT("No typing caps at AssemblePlus"), UScaffoldPolicy::ChooseLevel(&State, 0, false), EScaffoldLevel::AssemblePlus);
	State.Box = 5;
	TestEqual(TEXT("Box 5 -> Produce"), UScaffoldPolicy::ChooseLevel(&State, 0, true), EScaffoldLevel::Produce);
	TestEqual(TEXT("One error steps down"), UScaffoldPolicy::ChooseLevel(&State, 1, true), EScaffoldLevel::GapFill);
	TestEqual(TEXT("Errors floor at Recognize"), UScaffoldPolicy::ChooseLevel(nullptr, 5, true), EScaffoldLevel::Recognize);
	TestEqual(TEXT("New item after an error -> Recognize"), UScaffoldPolicy::ChooseLevel(nullptr, 1, true), EScaffoldLevel::Recognize);

	TestTrue(TEXT("Recognition pass is assisted"), UScaffoldPolicy::IsAssistedPass(EScaffoldLevel::Recognize, 1));
	TestTrue(TEXT("Second attempt is assisted"), UScaffoldPolicy::IsAssistedPass(EScaffoldLevel::AssemblePlus, 2));
	TestFalse(TEXT("First-try Assemble pass promotes"), UScaffoldPolicy::IsAssistedPass(EScaffoldLevel::Assemble, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyScaffoldMaterialTest, "LinguaSpy.Scaffold.Material", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyScaffoldMaterialTest::RunTest(const FString& Parameters)
{
	const FWordTileChallenge Cafe = LinguaSpyPhase5Test::MakeCafe();

	const TArray<FWordTile> Assemble = UScaffoldPolicy::BuildTiles(Cafe, EScaffoldLevel::Assemble);
	TestEqual(TEXT("Assemble drops distractors"), Assemble.Num(), 4);
	TestFalse(TEXT("No 'veux' tile"), Assemble.ContainsByPredicate([](const FWordTile& T) { return T.Text == TEXT("veux"); }));
	TestEqual(TEXT("AssemblePlus keeps all"), UScaffoldPolicy::BuildTiles(Cafe, EScaffoldLevel::AssemblePlus).Num(), 6);

	// Auto gap: the longest word ("voudrais")
	const TArray<int32> Gaps = UScaffoldPolicy::GetGapIndices(Cafe);
	TestEqual(TEXT("One gap for a short sentence"), Gaps.Num(), 1);
	TestEqual(TEXT("Gap is 'voudrais'"), Gaps.IsEmpty() ? -1 : Gaps[0], 1);
	TestEqual(TEXT("Gap display"), UScaffoldPolicy::BuildGapDisplay(Cafe, true), FString(TEXT("je ____ un café")));
	TestTrue(TEXT("Filled gap passes"), USentenceGrader::Grade(Cafe, UScaffoldPolicy::FillGaps(Cafe, { TEXT("voudrais") })).bPassed);
	TestFalse(TEXT("Wrong gap fails"), USentenceGrader::Grade(Cafe, UScaffoldPolicy::FillGaps(Cafe, { TEXT("veux") })).bPassed);

	FWordTileChallenge Authored = Cafe;
	Authored.GapTokenIndices = { 3 };
	TestEqual(TEXT("Authored gap wins"), UScaffoldPolicy::GetGapIndices(Authored)[0], 3);

	TArray<FString> Options;
	int32 Correct = INDEX_NONE;
	UScaffoldPolicy::BuildRecognitionOptions(Cafe, true, 1234, Options, Correct);
	TestEqual(TEXT("Three options"), Options.Num(), 3);
	TestTrue(TEXT("Correct index valid"), Options.IsValidIndex(Correct));
	if (Options.IsValidIndex(Correct))
	{
		TestEqual(TEXT("Correct option is the model"), Options[Correct], FString(TEXT("je voudrais un café")));
	}
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		if (i != Correct)
		{
			TestFalse(TEXT("Wrong option is not accepted"), USentenceGrader::Grade(Cafe, USentenceGrader::TokenizeTyped(Options[i])).bPassed);
		}
	}

	// No distractor tiles: falls back to word order swaps
	FWordTileChallenge NoDistractors = Cafe;
	NoDistractors.Tiles.SetNum(4);
	UScaffoldPolicy::BuildRecognitionOptions(NoDistractors, true, 7, Options, Correct);
	TestEqual(TEXT("Swaps still give 3 options"), Options.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyGraderTolerantTest, "LinguaSpy.GraderTolerant", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyGraderTolerantTest::RunTest(const FString& Parameters)
{
	const FWordTileChallenge Cafe = LinguaSpyPhase5Test::MakeCafe();

	const FGradeResult Accent = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("cafe") });
	TestFalse(TEXT("Missing accent still fails"), Accent.bPassed);
	TestTrue(TEXT("...but is flagged accent-only"), Accent.bAccentOnly);
	TestEqual(TEXT("First wrong is the accent word"), Accent.FirstWrongIndex, 3);

	const FGradeResult Wrong = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("veux"), TEXT("un"), TEXT("café") });
	TestFalse(TEXT("Wrong word is not accent-only"), Wrong.bAccentOnly);
	TestEqual(TEXT("First wrong index"), Wrong.FirstWrongIndex, 1);

	const FGradeResult Short = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("voudrais") });
	TestEqual(TEXT("Missing words point past the end"), Short.FirstWrongIndex, 2);

	const TArray<FString> Typed = USentenceGrader::TokenizeTyped(TEXT("  Je voudrais   un café ! "));
	TestEqual(TEXT("Typed tokens"), Typed.Num(), 4);
	TestTrue(TEXT("Typed sentence passes (case, spaces, punctuation)"), USentenceGrader::Grade(Cafe, Typed).bPassed);
	TestEqual(TEXT("Elision stays one token"), USentenceGrader::TokenizeTyped(TEXT("s'il vous plaît")).Num(), 3);
	TestEqual(TEXT("Fold accents"), USentenceGrader::FoldAccents(TEXT("Été à Noël")), FString(TEXT("Ete a Noel")));
	TestEqual(TEXT("Non-Latin untouched"), USentenceGrader::FoldAccents(TEXT("コーヒー")), FString(TEXT("コーヒー")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyFeedbackHintTest, "LinguaSpy.Feedback.Hint", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyFeedbackHintTest::RunTest(const FString& Parameters)
{
	FWordTileChallenge Cafe = LinguaSpyPhase5Test::MakeCafe();
	FChallengeHint General;
	General.TokenIndex = -1;
	General.Text = FText::FromString(TEXT("general"));
	FChallengeHint Verb;
	Verb.TokenIndex = 1;
	Verb.Text = FText::FromString(TEXT("verb"));
	Cafe.Hints = { General, Verb };

	const FGradeResult WrongVerb = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("veux"), TEXT("un"), TEXT("café") });
	TestEqual(TEXT("Token hint wins"), UScaffoldPolicy::ChooseHint(Cafe, WrongVerb, 4).ToString(), FString(TEXT("verb")));

	const FGradeResult WrongNoun = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("thé") });
	TestEqual(TEXT("General hint otherwise"), UScaffoldPolicy::ChooseHint(Cafe, WrongNoun, 4).ToString(), FString(TEXT("general")));

	const FGradeResult Accent = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("cafe") });
	TestTrue(TEXT("Accent hint beats authored hints"), UScaffoldPolicy::ChooseHint(Cafe, Accent, 4).ToString().Contains(TEXT("accent")));

	Cafe.Hints.Reset();
	const FGradeResult Short = USentenceGrader::Grade(Cafe, { TEXT("je"), TEXT("voudrais") });
	TestTrue(TEXT("Length hint"), UScaffoldPolicy::ChooseHint(Cafe, Short, 2).ToString().Contains(TEXT("4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyDebriefTest, "LinguaSpy.Debrief.Select", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyDebriefTest::RunTest(const FString& Parameters)
{
	const TArray<FMissionItemRecord> Records = {
		{ TEXT("clean1"), true, false },
		{ TEXT("assisted"), true, true },
		{ TEXT("failed"), false, false },
		{ TEXT("clean2"), true, false },
		{ TEXT("clean1"), false, false }, // later failed again -> worst result counts
	};
	const TArray<FString> Picks = UMissionSubsystem::SelectDebriefItems(Records, 3);
	TestEqual(TEXT("Max respected"), Picks.Num(), 3);
	TestEqual(TEXT("Failed first (answer order)"), Picks[0], FString(TEXT("clean1")));
	TestEqual(TEXT("Then other failures"), Picks[1], FString(TEXT("failed")));
	TestEqual(TEXT("Then assisted"), Picks[2], FString(TEXT("assisted")));
	TestEqual(TEXT("Empty in, empty out"), UMissionSubsystem::SelectDebriefItems({}, 4).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyHandlerAdaptTest, "LinguaSpy.Handler.Adapt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyHandlerAdaptTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("S fades support"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::Shadowing, 0), EHandlerMode::OnCall);
	TestEqual(TEXT("A fades support"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::OnCall, 1), EHandlerMode::RadioSilence);
	TestEqual(TEXT("Capped"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::RadioSilence, 0), EHandlerMode::RadioSilence);
	TestEqual(TEXT("B keeps"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::OnCall, 2), EHandlerMode::OnCall);
	TestEqual(TEXT("C restores support"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::RadioSilence, 3), EHandlerMode::OnCall);
	TestEqual(TEXT("Floor"), UMissionSubsystem::AdaptHandlerMode(EHandlerMode::Shadowing, 3), EHandlerMode::Shadowing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySaveMigrationV3Test, "LinguaSpy.SaveMigrationV3", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySaveMigrationV3Test::RunTest(const FString& Parameters)
{
	ULinguaSpySaveGame* Save = NewObject<ULinguaSpySaveGame>();
	Save->Version = 2;
	FCityProgress Paris;
	Paris.CompletedMissions = { TEXT("Paris_FirstContact") };
	Save->Cities.Add(TEXT("Paris"), Paris);
	Save->Cities.Add(TEXT("Tokyo"), FCityProgress());

	TestTrue(TEXT("v2 migrates"), ULinguaSpySaveGame::Migrate(Save));
	TestEqual(TEXT("Current"), Save->Version, ULinguaSpySaveGame::CurrentVersion);
	TestEqual(TEXT("Played city keeps tap-to-translate"), Save->GetHandlerMode(TEXT("Paris")), EHandlerMode::OnCall);
	TestEqual(TEXT("Unplayed city starts Shadowing"), Save->GetHandlerMode(TEXT("Rome")), EHandlerMode::Shadowing);
	TestTrue(TEXT("Finished mission counts as briefed"), Save->BriefingsCompleted.Contains(TEXT("Paris_FirstContact")));

	ULinguaSpySaveGame* Always = NewObject<ULinguaSpySaveGame>();
	Always->Version = 2;
	Always->Settings.bAlwaysShowTranslation = true;
	Always->Cities.Add(TEXT("Madrid"), FCityProgress());
	ULinguaSpySaveGame::Migrate(Always);
	TestEqual(TEXT("'Always show' becomes Shadowing"), Always->GetHandlerMode(TEXT("Madrid")), EHandlerMode::Shadowing);

	// Round trip of the new fields
	Save->ParticipantCode = TEXT("MT-07");
	Save->SeenFieldNotes = { TEXT("fr_voudrais") };
	Save->FieldNoteCounts.Add(TEXT("fr_vous"), 1);
	FAssessmentResult Result;
	Result.FormId = TEXT("FR_A");
	Result.Phase = TEXT("Pre");
	Result.Score = 9;
	Result.MaxScore = 15;
	FAssessmentItemResult ItemResult;
	ItemResult.ItemId = TEXT("A01");
	ItemResult.bCorrect = true;
	Result.Items.Add(ItemResult);
	Save->AssessmentResults.Add(Result);
	Save->Settings.VoiceVolume = 0.4f;

	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
	const ULinguaSpySaveGame* Loaded = Cast<ULinguaSpySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (TestNotNull(TEXT("Deserialize"), Loaded))
	{
		TestEqual(TEXT("Code"), Loaded->ParticipantCode, FString(TEXT("MT-07")));
		TestEqual(TEXT("Handler mode"), Loaded->GetHandlerMode(TEXT("Paris")), EHandlerMode::OnCall);
		TestTrue(TEXT("Seen note"), Loaded->SeenFieldNotes.Contains(TEXT("fr_voudrais")));
		TestEqual(TEXT("Note count"), Loaded->FieldNoteCounts.FindRef(TEXT("fr_vous")), 1);
		TestEqual(TEXT("Assessment"), Loaded->AssessmentResults.Num(), 1);
		TestEqual(TEXT("Assessment score"), Loaded->AssessmentResults.Num() ? Loaded->AssessmentResults[0].Score : -1, 9);
		TestEqual(TEXT("Voice volume"), Loaded->Settings.VoiceVolume, 0.4f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyAssessmentTest, "LinguaSpy.Assessment.Scoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyAssessmentTest::RunTest(const FString& Parameters)
{
	FAssessmentItem Choice;
	Choice.Type = EAssessmentItemType::MeaningChoose;
	Choice.CorrectIndex = 2;
	TestTrue(TEXT("Right choice"), UAssessmentAsset::ScoreItem(Choice, 2, {}));
	TestFalse(TEXT("Wrong choice"), UAssessmentAsset::ScoreItem(Choice, 1, {}));
	TestFalse(TEXT("No choice"), UAssessmentAsset::ScoreItem(Choice, INDEX_NONE, {}));

	FAssessmentItem Gap;
	Gap.Type = EAssessmentItemType::GapFill;
	Gap.Challenge = LinguaSpyPhase5Test::MakeCafe();
	TestTrue(TEXT("Gap right"), UAssessmentAsset::ScoreItem(Gap, INDEX_NONE, { TEXT("voudrais") }));
	TestFalse(TEXT("Gap wrong"), UAssessmentAsset::ScoreItem(Gap, INDEX_NONE, { TEXT("veux") }));
	FAssessmentItem GapAccent = Gap;
	GapAccent.Challenge.GapTokenIndices = { 3 };
	TestTrue(TEXT("Typed gap ignores accents"), UAssessmentAsset::ScoreItem(GapAccent, INDEX_NONE, { TEXT("cafe") }));

	FAssessmentItem Assemble;
	Assemble.Type = EAssessmentItemType::Assemble;
	Assemble.Challenge = LinguaSpyPhase5Test::MakeCafe();
	TestTrue(TEXT("Assemble right"), UAssessmentAsset::ScoreItem(Assemble, INDEX_NONE, { TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café") }));
	TestFalse(TEXT("Assemble order"), UAssessmentAsset::ScoreItem(Assemble, INDEX_NONE, { TEXT("voudrais"), TEXT("je"), TEXT("un"), TEXT("café") }));

	// Counterbalancing: pre and delayed share a form, post uses the other; codes split across both orders
	using namespace LinguaSpyAssessmentPhase;
	int32 FirstIsZero = 0;
	for (const TCHAR* Code : { TEXT("MT-01"), TEXT("MT-02"), TEXT("MT-03"), TEXT("MT-04"), TEXT("MT-05"), TEXT("MT-06") })
	{
		const int32 PreForm = UAssessmentSubsystem::ChooseFormIndex(2, Code, Pre);
		TestEqual(TEXT("Delayed = pre form"), UAssessmentSubsystem::ChooseFormIndex(2, Code, Delayed), PreForm);
		TestEqual(TEXT("Post = other form"), UAssessmentSubsystem::ChooseFormIndex(2, Code, Post), 1 - PreForm);
		TestEqual(TEXT("Stable"), UAssessmentSubsystem::ChooseFormIndex(2, FString(Code).ToLower(), Pre), PreForm);
		FirstIsZero += PreForm == 0 ? 1 : 0;
	}
	TestTrue(TEXT("Both orders occur"), FirstIsZero > 0 && FirstIsZero < 6);
	TestEqual(TEXT("Single form"), UAssessmentSubsystem::ChooseFormIndex(1, TEXT("X"), Post), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyTelemetryFormatTest, "LinguaSpy.Telemetry.Format", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyTelemetryFormatTest::RunTest(const FString& Parameters)
{
	const FString Line = ULearningTelemetrySubsystem::FormatLine(
		FTelemetryEvent(TEXT("challenge")).Str(TEXT("item"), TEXT("/Game/X.X#n1#-1")).Bool(TEXT("passed"), true).Int(TEXT("ms"), 1520)
			.Str(TEXT("answer"), TEXT("café \"quoted\"")),
		TEXT("20261004-120000"), TEXT("MT-07"), FDateTime(2026, 10, 4, 12, 0, 5), 5000);

	TestFalse(TEXT("One line"), Line.Contains(TEXT("\n")));
	TSharedPtr<FJsonObject> Parsed;
	TestTrue(TEXT("Valid JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Line), Parsed) && Parsed.IsValid());
	if (Parsed.IsValid())
	{
		TestEqual(TEXT("Event"), Parsed->GetStringField(TEXT("event")), FString(TEXT("challenge")));
		TestEqual(TEXT("Participant"), Parsed->GetStringField(TEXT("participant")), FString(TEXT("MT-07")));
		TestEqual(TEXT("Session time"), static_cast<int32>(Parsed->GetNumberField(TEXT("t_ms"))), 5000);
		TestEqual(TEXT("Event field"), static_cast<int32>(Parsed->GetNumberField(TEXT("ms"))), 1520);
		TestTrue(TEXT("Passed"), Parsed->GetBoolField(TEXT("passed")));
		TestEqual(TEXT("Unicode + quotes survive"), Parsed->GetStringField(TEXT("answer")), FString(TEXT("café \"quoted\"")));
	}

	const FString Anon = ULearningTelemetrySubsystem::FormatLine(FTelemetryEvent(TEXT("x")), TEXT("s"), FString(), FDateTime(2026, 1, 1), 0);
	TestTrue(TEXT("Anonymous participant"), Anon.Contains(TEXT("\"participant\":\"anon\"")));

	TestTrue(TEXT("Code ok"), ULinguaSpySaveSubsystem::IsValidParticipantCode(TEXT("MT-07")));
	TestFalse(TEXT("No spaces (names)"), ULinguaSpySaveSubsystem::IsValidParticipantCode(TEXT("Jane Doe")));
	TestFalse(TEXT("Max 8"), ULinguaSpySaveSubsystem::IsValidParticipantCode(TEXT("ABCDEFGHI")));
	TestFalse(TEXT("Not empty"), ULinguaSpySaveSubsystem::IsValidParticipantCode(TEXT("")));
	return true;
}

#endif
