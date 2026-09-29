// LinguaSpy - automation tests for the word-tile grader

#include "Misc/AutomationTest.h"
#include "SentenceGrader.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace LinguaSpyTests
{
	FWordTileChallenge MakeCoffeeChallenge()
	{
		FWordTileChallenge Challenge;
		Challenge.TileBank = { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît"), TEXT("veux"), TEXT("thé") };

		FAcceptedAnswer Main;
		Main.Tokens = { TEXT("Bonjour"), TEXT("je"), TEXT("voudrais"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") };
		FAcceptedAnswer Alt;
		Alt.Tokens = { TEXT("Bonjour"), TEXT("un"), TEXT("café"), TEXT("s'il"), TEXT("vous"), TEXT("plaît") };
		Challenge.AcceptedAnswers = { Main, Alt };
		return Challenge;
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
		TestEqual(TEXT("JoinTokens joins elided apostrophes"),
			USentenceGrader::JoinTokens({ TEXT("s'"), TEXT("il"), TEXT("vous"), TEXT("plaît") }), FString(TEXT("s'il vous plaît")));
	}
	return true;
}

#endif
