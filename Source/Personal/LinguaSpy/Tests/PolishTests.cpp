// LinguaSpy - automation tests for the visual-polish branch: look preset blending and the playtest bug fixes
// (briefing quick-check counter, mission tally buckets, hint wording per scaffold level)

#include "Misc/AutomationTest.h"
#include "BriefingWidget.h"
#include "CityLook.h"
#include "FluencyScoring.h"
#include "LinguaSpySaveGame.h"
#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace LinguaSpyPolishTest
{
	FCityLook MakeLook(float Value, const FLinearColor& Color, float SunYaw)
	{
		FCityLook Look;
		Look.SunRotation = FRotator(-30.f, SunYaw, 0.f);
		Look.SunColor = Color;
		Look.SunIntensity = Value;
		Look.FogDensity = Value / 100.f;
		Look.WhiteTemp = 5000.f + Value * 100.f;
		Look.Saturation = FVector4(Value, Value, Value, Value);
		Look.DofFstop = Value;
		return Look;
	}

	// "コーヒー|を|ください": three unspaced chunk tiles, like the Tokyo content
	FWordTileChallenge MakeKonbini()
	{
		FWordTileChallenge Challenge;
		for (const TCHAR* Chunk : { TEXT("コーヒー"), TEXT("を"), TEXT("ください"), TEXT("お茶") })
		{
			FWordTile Tile;
			Tile.Text = Chunk;
			Challenge.Tiles.Add(Tile);
		}
		FAcceptedAnswer Answer;
		Answer.Tokens = { TEXT("コーヒー"), TEXT("を"), TEXT("ください") };
		Challenge.AcceptedAnswers.Add(Answer);
		FChallengeHint TokenHint;
		TokenHint.TokenIndex = 0;
		TokenHint.Text = FText::FromString(TEXT("first chunk"));
		Challenge.Hints.Add(TokenHint);
		return Challenge;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyLookBlendTest, "LinguaSpy.Look.Blend", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyLookBlendTest::RunTest(const FString& Parameters)
{
	using namespace LinguaSpyPolishTest;
	FCityLook A = MakeLook(2.f, FLinearColor(1.f, 0.f, 0.f), 350.f);
	FCityLook B = MakeLook(6.f, FLinearColor(0.f, 0.f, 1.f), 10.f);
	A.ToonScalars.Add(TEXT("HazeMax"), 0.2f);
	A.ToonScalars.Add(TEXT("OnlyA"), 7.f);
	B.ToonScalars.Add(TEXT("HazeMax"), 0.6f);
	B.ToonScalars.Add(TEXT("OnlyB"), 9.f);
	A.ToonColors.Add(TEXT("HazeColor"), FLinearColor::Black);
	B.ToonColors.Add(TEXT("HazeColor"), FLinearColor::White);

	const FCityLook Start = UCityLookLibrary::BlendLooks(A, B, 0.f);
	TestEqual(TEXT("Alpha 0 = A (intensity)"), Start.SunIntensity, 2.f);
	TestTrue(TEXT("Alpha 0 = A (colour)"), Start.SunColor.Equals(A.SunColor));

	const FCityLook End = UCityLookLibrary::BlendLooks(A, B, 1.f);
	TestEqual(TEXT("Alpha 1 = B (intensity)"), End.SunIntensity, 6.f);
	TestEqual(TEXT("Alpha 1 = B (f-stop)"), End.DofFstop, 6.f);

	const FCityLook Mid = UCityLookLibrary::BlendLooks(A, B, 0.5f);
	TestEqual(TEXT("Scalars lerp"), Mid.SunIntensity, 4.f);
	TestEqual(TEXT("White balance lerps"), Mid.WhiteTemp, 5400.f);
	TestTrue(TEXT("Grading vectors lerp"), FVector4(Mid.Saturation).Equals(FVector4(4.f, 4.f, 4.f, 4.f)));
	TestTrue(TEXT("Colours lerp in linear space"), Mid.SunColor.Equals(FLinearColor(0.5f, 0.f, 0.5f)));
	TestEqual(TEXT("Toon scalar keys in both sides lerp"), Mid.ToonScalars.FindRef(TEXT("HazeMax")), 0.4f, 1e-4f);
	TestEqual(TEXT("A-only key keeps A's value"), Mid.ToonScalars.FindRef(TEXT("OnlyA")), 7.f);
	TestEqual(TEXT("B-only key keeps B's value"), Mid.ToonScalars.FindRef(TEXT("OnlyB")), 9.f);
	TestTrue(TEXT("Toon colours lerp"), Mid.ToonColors.FindRef(TEXT("HazeColor")).Equals(FLinearColor(0.5f, 0.5f, 0.5f, 1.f)));

	// Sun yaw 350 -> 10 must go the short way (through 0), not swing 340 degrees through 180
	const float MidYaw = FRotator::NormalizeAxis(Mid.SunRotation.Yaw);
	TestTrue(TEXT("Sun rotation takes the shortest arc"), FMath::Abs(MidYaw) < 0.5f);
	TestEqual(TEXT("Sun pitch kept"), Mid.SunRotation.Pitch, -30.0, 0.01);

	const FCityLook Over = UCityLookLibrary::BlendLooks(A, B, 3.f);
	TestEqual(TEXT("Alpha clamps above 1"), Over.SunIntensity, 6.f);
	const FCityLook Under = UCityLookLibrary::BlendLooks(A, B, -1.f);
	TestEqual(TEXT("Alpha clamps below 0"), Under.SunIntensity, 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyBriefingQuizStepTest, "LinguaSpy.Briefing.QuizStep", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyBriefingQuizStepTest::RunTest(const FString& Parameters)
{
	// 16 first-round items; one miss on item 1 appends a retry (17 items in the array)
	FBriefingQuizStep First = UBriefingWidget::GetQuizStep(0, 16, 16);
	TestEqual(TEXT("1 / 16"), First.Number, 1);
	TestEqual(TEXT("of 16"), First.Count, 16);
	FBriefingQuizStep AfterMiss = UBriefingWidget::GetQuizStep(1, 16, 17);
	TestEqual(TEXT("2 / 16 after a miss (was 2 / 17)"), AfterMiss.Count, 16);
	TestEqual(TEXT("2nd item"), AfterMiss.Number, 2);
	TestFalse(TEXT("Still the first round"), AfterMiss.bRetryRound);

	FBriefingQuizStep LastFirst = UBriefingWidget::GetQuizStep(15, 16, 18);
	TestEqual(TEXT("16 / 16"), LastFirst.Number, 16);
	FBriefingQuizStep Retry = UBriefingWidget::GetQuizStep(16, 16, 18);
	TestTrue(TEXT("Retry round"), Retry.bRetryRound);
	TestEqual(TEXT("Second look 1 / 2"), Retry.Number, 1);
	TestEqual(TEXT("of 2"), Retry.Count, 2);
	FBriefingQuizStep Retry2 = UBriefingWidget::GetQuizStep(17, 16, 18);
	TestEqual(TEXT("Second look 2 / 2"), Retry2.Number, 2);

	FBriefingQuizStep Empty = UBriefingWidget::GetQuizStep(0, 0, 0);
	TestEqual(TEXT("Empty quiz has no count"), Empty.Count, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyFluencyTallyTest, "LinguaSpy.FluencyTally", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyFluencyTallyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("First try"), UFluencyScoring::ClassifyAnswer(true, 1), EAnswerOutcome::Clean);
	TestEqual(TEXT("Right after the hint"), UFluencyScoring::ClassifyAnswer(true, 2), EAnswerOutcome::AfterHint);
	TestEqual(TEXT("Wrong twice"), UFluencyScoring::ClassifyAnswer(false, 2), EAnswerOutcome::Missed);

	// The trailer capture: one wrong attempt then fixed, one tap-to-translate, two clean answers.
	// Before the fix the debrief said "0 mistakes, 2 translations peeked" (the retry was counted as a peek).
	FMissionGrade Tally;
	Tally.Challenges = 3;
	Tally.Peeks = 1;
	for (const EAnswerOutcome Outcome : { UFluencyScoring::ClassifyAnswer(true, 2), UFluencyScoring::ClassifyAnswer(true, 1),
		UFluencyScoring::ClassifyAnswer(true, 1) })
	{
		Tally.Mistakes += Outcome == EAnswerOutcome::Missed ? 1 : 0;
		Tally.Retries += Outcome == EAnswerOutcome::AfterHint ? 1 : 0;
	}
	TestEqual(TEXT("No mistakes"), Tally.Mistakes, 0);
	TestEqual(TEXT("One fixed after a hint"), Tally.Retries, 1);
	TestEqual(TEXT("One translation peeked"), Tally.Peeks, 1);
	TestEqual(TEXT("Help = peeks + retries"), Tally.GetHelpCount(), 2);
	// Same grade as before the split: reliance = 0.35 * 2 / 3 = 0.23 -> A
	TestEqual(TEXT("Grade unchanged by the split"), UFluencyScoring::ComputeGrade(Tally.Challenges, Tally.Mistakes, Tally.GetHelpCount()), 1);

	ULinguaSpySaveGame* Save = NewObject<ULinguaSpySaveGame>();
	Save->Version = 3;
	TestTrue(TEXT("v3 migrates to v4"), ULinguaSpySaveGame::Migrate(Save));
	TestEqual(TEXT("Now v4"), Save->Version, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyHintLevelsTest, "LinguaSpy.Feedback.HintLevels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyHintLevelsTest::RunTest(const FString& Parameters)
{
	const FWordTileChallenge Konbini = LinguaSpyPolishTest::MakeKonbini();

	// Recognition grades as "first wrong = 0" with 0 submitted tokens: it used to say "The phrase has 3 words"
	FGradeResult Recognition;
	Recognition.bPassed = false;
	Recognition.ClosestAnswerIndex = 0;
	Recognition.FirstWrongIndex = 0;
	const FString PickHint = UScaffoldPolicy::ChooseHint(Konbini, Recognition, 0, EScaffoldLevel::Recognize, false).ToString();
	TestFalse(TEXT("Recognize: no word count"), PickHint.Contains(TEXT("words")) || PickHint.Contains(TEXT("3")));
	TestFalse(TEXT("Recognize: token hints don't apply"), PickHint.Contains(TEXT("first chunk")));
	TestTrue(TEXT("Recognize: pick-the-sentence wording"), PickHint.Contains(TEXT("pick")));

	// Building from tiles in an unspaced language counts tiles
	FWordTileChallenge NoHints = Konbini;
	NoHints.Hints.Reset();
	const FGradeResult Short = USentenceGrader::Grade(NoHints, { TEXT("コーヒー") });
	const FString TileHint = UScaffoldPolicy::ChooseHint(NoHints, Short, 1, EScaffoldLevel::AssemblePlus, false).ToString();
	TestTrue(TEXT("Unspaced: counts tiles"), TileHint.Contains(TEXT("3 tiles")));
	TestFalse(TEXT("Unspaced: not words"), TileHint.Contains(TEXT("word")));

	// Spaced languages keep the original wording (LinguaSpy.Feedback.Hint covers the rest)
	const FString WordHint = UScaffoldPolicy::ChooseHint(NoHints, Short, 1, EScaffoldLevel::AssemblePlus, true).ToString();
	TestTrue(TEXT("Spaced: counts words"), WordHint.Contains(TEXT("3 words")));
	return true;
}

#endif
