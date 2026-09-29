// LinguaSpy - automation tests for the word-tile grader, mission transitions and save data

#include "Misc/AutomationTest.h"
#include "SentenceGrader.h"
#include "MissionAsset.h"
#include "LinguaSpySaveGame.h"
#include "NpcCharacter.h"
#include "ConversationAsset.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace LinguaSpyTests
{
	FWordTileChallenge MakeChallenge(const TArray<FString>& Tiles, const TArray<TArray<FString>>& Answers)
	{
		FWordTileChallenge Challenge;
		for (const FString& Text : Tiles)
		{
			FWordTile Tile;
			Tile.Text = Text;
			Challenge.Tiles.Add(Tile);
		}
		for (const TArray<FString>& Tokens : Answers)
		{
			FAcceptedAnswer Answer;
			Answer.Tokens = Tokens;
			Challenge.AcceptedAnswers.Add(Answer);
		}
		return Challenge;
	}

	FWordTileChallenge MakeCoffeeChallenge()
	{
		return MakeChallenge(
			{ TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît"), TEXT("veux"), TEXT("thé") },
			{
				{ TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") },
				{ TEXT("Bonjour"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") }
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyGraderTest, "LinguaSpy.Grader", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyGraderTest::RunTest(const FString& Parameters)
{
	using namespace LinguaSpyTests;
	const FWordTileChallenge Challenge = MakeCoffeeChallenge();

	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") });
		TestTrue(TEXT("Exact match passes"), R.bPassed);
		TestEqual(TEXT("Exact match uses first answer"), R.ClosestAnswerIndex, 0);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") });
		TestTrue(TEXT("Alternate accepted answer passes"), R.bPassed);
		TestEqual(TEXT("Alternate answer index"), R.ClosestAnswerIndex, 1);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("bonjour,"), TEXT("JE"), TEXT("voudrais"), TEXT("un"), TEXT("CAFÉ"), TEXT("s’il"), TEXT("vous"), TEXT("plaît.") });
		TestTrue(TEXT("Case, punctuation and apostrophe style are ignored"), R.bPassed);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("cafe"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") });
		TestFalse(TEXT("Missing accent fails"), R.bPassed);
		TestFalse(TEXT("Accentless tile marked wrong"), R.TileCorrect[4]);
		TestTrue(TEXT("Other tiles marked correct"), R.TileCorrect[3]);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("je"), TEXT("veux"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") });
		TestFalse(TEXT("Distractor fails"), R.bPassed);
		TestFalse(TEXT("Distractor tile marked wrong"), R.TileCorrect[2]);
		TestEqual(TEXT("Closest answer is the main one"), R.ClosestAnswerIndex, 0);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café") });
		TestFalse(TEXT("Missing tiles fail"), R.bPassed);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît"), TEXT("thé") });
		TestFalse(TEXT("Extra tiles fail"), R.bPassed);
		TestFalse(TEXT("Extra tile marked wrong"), R.TileCorrect[8]);
	}
	{
		const FGradeResult R = USentenceGrader::Grade(Challenge, {});
		TestFalse(TEXT("Empty answer fails"), R.bPassed);
	}
	{
		TestEqual(TEXT("JoinTokens keeps elided tiles whole"),
			USentenceGrader::JoinTokens({ TEXT("s'il"), TEXT("vous"), TEXT("plaît") }), FString(TEXT("s'il vous plaît")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyGraderLanguagesTest, "LinguaSpy.GraderLanguages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyGraderLanguagesTest::RunTest(const FString& Parameters)
{
	using namespace LinguaSpyTests;

	// Spanish: inverted punctuation is ignored, accents matter
	{
		const FWordTileChallenge Spanish = MakeChallenge({ TEXT("¿Dónde"), TEXT("está"), TEXT("el"), TEXT("metro?") },
			{ { TEXT("Dónde"), TEXT("está"), TEXT("el"), TEXT("metro") } });
		TestTrue(TEXT("Spanish with ¿? passes"), USentenceGrader::Grade(Spanish, { TEXT("¿Dónde"), TEXT("está"), TEXT("el"), TEXT("metro?") }).bPassed);
		TestFalse(TEXT("Spanish missing accent fails"), USentenceGrader::Grade(Spanish, { TEXT("Donde"), TEXT("está"), TEXT("el"), TEXT("metro") }).bPassed);
	}

	// Japanese: chunk tokens, full-width punctuation ignored, no spaces when joining
	{
		const FWordTileChallenge Japanese = MakeChallenge({ TEXT("コーヒー"), TEXT("を"), TEXT("ください"), TEXT("が") },
			{ { TEXT("コーヒー"), TEXT("を"), TEXT("ください") } });
		TestTrue(TEXT("Japanese exact passes"), USentenceGrader::Grade(Japanese, { TEXT("コーヒー"), TEXT("を"), TEXT("ください。") }).bPassed);
		TestFalse(TEXT("Japanese wrong particle fails"), USentenceGrader::Grade(Japanese, { TEXT("コーヒー"), TEXT("が"), TEXT("ください") }).bPassed);
		TestEqual(TEXT("Japanese joins without spaces"),
			USentenceGrader::JoinTokens({ TEXT("コーヒー"), TEXT("を"), TEXT("ください") }, false), FString(TEXT("コーヒーをください")));
	}

	// Italian: truncation/elision apostrophes stay inside their tile
	{
		TestEqual(TEXT("Italian apostrophes"), USentenceGrader::JoinTokens({ TEXT("un"), TEXT("po'"), TEXT("d'acqua") }), FString(TEXT("un po' d'acqua")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyMissionTransitionTest, "LinguaSpy.MissionTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyMissionTransitionTest::RunTest(const FString& Parameters)
{
	FMissionObjective Kiosk;
	Kiosk.ObjectiveId = TEXT("Kiosk");

	FObjectiveTransition Suspicious;
	Suspicious.RequiredOutcome = TEXT("Suspicious");
	Suspicious.NextObjective = TEXT("Metro");
	Kiosk.Transitions.Add(Suspicious);

	FObjectiveTransition Trusted;
	Trusted.RequiredFlags = { TEXT("KnowsBakery") };
	Trusted.BlockedFlags = { TEXT("Burned") };
	Trusted.NextObjective = TEXT("Bakery");
	Kiosk.Transitions.Add(Trusted);

	FObjectiveTransition Fallback;
	Fallback.NextObjective = UMissionAsset::MissionFailed;
	Kiosk.Transitions.Add(Fallback);

	TestEqual(TEXT("Outcome match wins first"), UMissionAsset::ResolveTransition(Kiosk, TEXT("Suspicious"), { TEXT("KnowsBakery") }), FName(TEXT("Metro")));
	TestEqual(TEXT("Required flag"), UMissionAsset::ResolveTransition(Kiosk, TEXT("Completed"), { TEXT("KnowsBakery") }), FName(TEXT("Bakery")));
	TestEqual(TEXT("Blocked flag falls through"), UMissionAsset::ResolveTransition(Kiosk, TEXT("Completed"), { TEXT("KnowsBakery"), TEXT("Burned") }), UMissionAsset::MissionFailed);
	TestEqual(TEXT("No flags falls through"), UMissionAsset::ResolveTransition(Kiosk, TEXT("Completed"), {}), UMissionAsset::MissionFailed);

	FMissionObjective Empty;
	TestEqual(TEXT("No transitions -> stays"), UMissionAsset::ResolveTransition(Empty, TEXT("Completed"), {}), FName(NAME_None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySaveRoundTripTest, "LinguaSpy.SaveRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySaveRoundTripTest::RunTest(const FString& Parameters)
{
	ULinguaSpySaveGame* Save = NewObject<ULinguaSpySaveGame>();
	FSkillStats Stats;
	Stats.Passes = 3;
	Stats.Failures = 5;
	Save->SkillStats.Add(TEXT("Paris.Ordering"), Stats);

	FCityProgress Paris;
	Paris.ActiveMission = TEXT("Croissant");
	Paris.CurrentObjective = TEXT("Kiosk");
	Paris.MissionFlags = { TEXT("KnowsBakery") };
	Paris.CompletedMissions = { TEXT("Tutorial") };
	Save->Cities.Add(TEXT("Paris"), Paris);
	Save->Settings.bShowReadings = false;

	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
	const ULinguaSpySaveGame* Loaded = Cast<ULinguaSpySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Deserialize"), Loaded))
	{
		return false;
	}

	TestEqual(TEXT("Skill failures"), Loaded->SkillStats.FindRef(TEXT("Paris.Ordering")).Failures, 5);
	const FCityProgress* LoadedParis = Loaded->Cities.Find(TEXT("Paris"));
	if (TestNotNull(TEXT("City progress"), LoadedParis))
	{
		TestEqual(TEXT("Objective"), LoadedParis->CurrentObjective, FName(TEXT("Kiosk")));
		TestTrue(TEXT("Flag"), LoadedParis->MissionFlags.Contains(TEXT("KnowsBakery")));
		TestEqual(TEXT("Completed"), LoadedParis->CompletedMissions.Num(), 1);
	}
	TestFalse(TEXT("Setting"), Loaded->Settings.bShowReadings);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyNpcConversationResolveTest, "LinguaSpy.NpcConversationResolve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyNpcConversationResolveTest::RunTest(const FString& Parameters)
{
	UConversationAsset* Retry = NewObject<UConversationAsset>();
	UConversationAsset* Mission2 = NewObject<UConversationAsset>();
	UConversationAsset* TargetOnly = NewObject<UConversationAsset>();

	TArray<FNpcConversationOption> Options;
	FNpcConversationOption RetryOption;
	RetryOption.Conversation = Retry;
	RetryOption.RequiredFlags = { TEXT("LostTail") };
	Options.Add(RetryOption);

	FNpcConversationOption Mission2Option;
	Mission2Option.Conversation = Mission2;
	Mission2Option.RequiredMission = TEXT("Paris_Croissant");
	Options.Add(Mission2Option);

	FNpcConversationOption TargetOption;
	TargetOption.Conversation = TargetOnly;
	TargetOption.bOnlyIfCurrentTarget = true;
	TargetOption.BlockedFlags = { TEXT("Done") };
	Options.Add(TargetOption);

	auto NoFlags = [](FName) { return false; };
	auto LostTail = [](FName Flag) { return Flag == TEXT("LostTail"); };
	auto Done = [](FName Flag) { return Flag == TEXT("Done"); };

	TestEqual(TEXT("Flag option wins first"), ANpcCharacter::ChooseConversationOption(Options, TEXT("Paris_Croissant"), true, LostTail), 0);
	TestEqual(TEXT("Mission option"), ANpcCharacter::ChooseConversationOption(Options, TEXT("Paris_Croissant"), false, NoFlags), 1);
	TestEqual(TEXT("Wrong mission skips"), ANpcCharacter::ChooseConversationOption(Options, TEXT("Paris_FirstContact"), false, NoFlags), INDEX_NONE);
	TestEqual(TEXT("Target-only option"), ANpcCharacter::ChooseConversationOption(Options, TEXT("Paris_FirstContact"), true, NoFlags), 2);
	TestEqual(TEXT("Blocked flag skips"), ANpcCharacter::ChooseConversationOption(Options, TEXT("Paris_FirstContact"), true, Done), INDEX_NONE);
	return true;
}

#endif
