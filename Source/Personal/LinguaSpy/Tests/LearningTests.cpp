// LinguaSpy - automation tests for spaced repetition, save migration and fluency grading

#include "Misc/AutomationTest.h"
#include "SrsScheduler.h"
#include "FluencyScoring.h"
#include "LinguaSpySaveGame.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySrsScheduleTest, "LinguaSpy.Srs.Schedule", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySrsScheduleTest::RunTest(const FString& Parameters)
{
	const FDateTime T0(2026, 10, 1, 12, 0, 0);
	FSrsItemState Item;

	// First unaided pass: box 0 -> 1, due in 10 minutes
	USrsScheduler::Update(Item, true, false, T0);
	TestEqual(TEXT("Pass moves up a box"), Item.Box, 1);
	TestFalse(TEXT("Not due right away"), USrsScheduler::IsDue(Item, T0 + FTimespan::FromMinutes(9.0)));
	TestTrue(TEXT("Due after 10 minutes"), USrsScheduler::IsDue(Item, T0 + FTimespan::FromMinutes(10.0)));

	// Box 1 -> 2 waits a day
	const FDateTime T1 = T0 + FTimespan::FromMinutes(15.0);
	USrsScheduler::Update(Item, true, false, T1);
	TestEqual(TEXT("Second pass -> box 2"), Item.Box, 2);
	TestFalse(TEXT("Box 2 not due after 23h"), USrsScheduler::IsDue(Item, T1 + FTimespan::FromHours(23.0)));
	TestTrue(TEXT("Box 2 due after 1 day"), USrsScheduler::IsDue(Item, T1 + FTimespan::FromDays(1.0)));

	// Assisted pass keeps the box but reschedules
	const FDateTime T2 = T1 + FTimespan::FromDays(2.0);
	USrsScheduler::Update(Item, true, true, T2);
	TestEqual(TEXT("Assisted pass keeps box"), Item.Box, 2);
	TestEqual(TEXT("Assisted pass counted"), Item.AssistedPasses, 1);
	TestEqual(TEXT("Rescheduled from now"), Item.DueUtcTicks, (T2 + FTimespan::FromDays(1.0)).GetTicks());

	// Fail resets to box 1
	USrsScheduler::Update(Item, false, false, T2);
	TestEqual(TEXT("Fail -> box 1"), Item.Box, 1);
	TestEqual(TEXT("Failure counted"), Item.Failures, 1);
	TestEqual(TEXT("Passes counted"), Item.Passes, 3);

	// Box is capped
	FSrsItemState Strong;
	for (int32 i = 0; i < 10; ++i)
	{
		USrsScheduler::Update(Strong, true, false, T0);
	}
	TestEqual(TEXT("Box capped at MaxBox"), Strong.Box, USrsScheduler::MaxBox);
	TestEqual(TEXT("Full strength"), USrsScheduler::GetStrength(Strong), 1.f);

	// Selection: due first (most overdue first), city filter, then weakest non-due as filler
	TMap<FString, FSrsItemState> Items;
	auto Add = [&Items](const TCHAR* Key, FName City, int32 Box, const FDateTime& Due, int32 Failures = 0)
	{
		FSrsItemState State;
		State.CityId = City;
		State.Box = Box;
		State.DueUtcTicks = Due.GetTicks();
		State.Failures = Failures;
		Items.Add(Key, State);
	};
	const FDateTime Now = T0 + FTimespan::FromDays(10.0);
	Add(TEXT("a"), TEXT("Paris"), 3, Now - FTimespan::FromHours(1.0));
	Add(TEXT("b"), TEXT("Paris"), 1, Now - FTimespan::FromDays(2.0));
	Add(TEXT("c"), TEXT("Paris"), 2, Now + FTimespan::FromDays(1.0), 3);
	Add(TEXT("d"), TEXT("Paris"), 4, Now + FTimespan::FromDays(5.0));
	Add(TEXT("e"), TEXT("Tokyo"), 1, Now - FTimespan::FromDays(5.0));

	TestEqual(TEXT("Due count Paris"), USrsScheduler::CountDue(Items, TEXT("Paris"), Now), 2);
	TestEqual(TEXT("Due count all"), USrsScheduler::CountDue(Items, NAME_None, Now), 3);

	const TArray<FString> DueOnly = USrsScheduler::SelectReviewItems(Items, TEXT("Paris"), Now, 10, false);
	TestEqual(TEXT("Only due items"), DueOnly.Num(), 2);
	if (DueOnly.Num() == 2)
	{
		TestEqual(TEXT("Most overdue first"), DueOnly[0], FString(TEXT("b")));
		TestEqual(TEXT("Then next"), DueOnly[1], FString(TEXT("a")));
	}
	const TArray<FString> Filled = USrsScheduler::SelectReviewItems(Items, TEXT("Paris"), Now, 3, true);
	TestEqual(TEXT("Filled to max"), Filled.Num(), 3);
	if (Filled.Num() == 3)
	{
		TestEqual(TEXT("Filler is the weakest non-due item"), Filled[2], FString(TEXT("c")));
	}
	TestEqual(TEXT("Max respected"), USrsScheduler::SelectReviewItems(Items, TEXT("Paris"), Now, 1, true).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySrsItemKeyTest, "LinguaSpy.Srs.ItemKey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySrsItemKeyTest::RunTest(const FString& Parameters)
{
	FSrsItemKey Key;
	Key.Conversation = FSoftObjectPath(TEXT("/Game/LinguaSpy/Data/French/CV_Paris_Cafe.CV_Paris_Cafe"));
	Key.NodeId = TEXT("Order");
	Key.ChoiceIndex = 2;

	FSrsItemKey Parsed;
	TestTrue(TEXT("Parses"), FSrsItemKey::Parse(Key.ToString(), Parsed));
	TestTrue(TEXT("Round trip"), Parsed == Key);

	Key.ChoiceIndex = INDEX_NONE;
	TestTrue(TEXT("Parses INDEX_NONE"), FSrsItemKey::Parse(Key.ToString(), Parsed));
	TestEqual(TEXT("Choice INDEX_NONE"), Parsed.ChoiceIndex, static_cast<int32>(INDEX_NONE));

	TestFalse(TEXT("Rejects garbage"), FSrsItemKey::Parse(TEXT("nonsense"), Parsed));
	TestFalse(TEXT("Rejects missing node"), FSrsItemKey::Parse(TEXT("/Game/X.X##1"), Parsed));
	TestFalse(TEXT("Rejects bad index"), FSrsItemKey::Parse(TEXT("/Game/X.X#Node#abc"), Parsed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySaveMigrationTest, "LinguaSpy.SaveMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySaveMigrationTest::RunTest(const FString& Parameters)
{
	ULinguaSpySaveGame* Save = NewObject<ULinguaSpySaveGame>();
	Save->Version = 1;
	FSkillStats Stats;
	Stats.Passes = 4;
	Save->SkillStats.Add(TEXT("Rome.Ordering"), Stats);
	FCityProgress Rome;
	Rome.CompletedMissions = { TEXT("Rome_Espresso") };
	Save->Cities.Add(TEXT("Rome"), Rome);
	Save->Settings.bShowReadings = false;
	Save->Settings.bAlwaysShowTranslation = true;

	TestTrue(TEXT("v1 migrates"), ULinguaSpySaveGame::Migrate(Save));
	TestEqual(TEXT("Now current version"), Save->Version, ULinguaSpySaveGame::CurrentVersion);
	TestEqual(TEXT("Skill stats kept"), Save->SkillStats.FindRef(TEXT("Rome.Ordering")).Passes, 4);
	TestEqual(TEXT("Missions kept"), Save->Cities.FindRef(TEXT("Rome")).CompletedMissions.Num(), 1);
	TestFalse(TEXT("Readings setting kept"), Save->Settings.bShowReadings);
	TestFalse(TEXT("Tap-to-translate is the new default"), Save->Settings.bAlwaysShowTranslation);
	TestTrue(TEXT("No SRS items yet"), Save->SrsItems.IsEmpty());
	TestEqual(TEXT("Default master volume"), Save->Settings.MasterVolume, FLinguaSpySettings().MasterVolume);

	TestFalse(TEXT("Current version is left alone"), ULinguaSpySaveGame::Migrate(Save));

	// A v1 file never stored its version (it equalled the old default), so it loads as 0
	ULinguaSpySaveGame* Unversioned = NewObject<ULinguaSpySaveGame>();
	TestEqual(TEXT("Unset version is 0"), Unversioned->Version, 0);
	TestTrue(TEXT("Unversioned file migrates"), ULinguaSpySaveGame::Migrate(Unversioned));
	TestEqual(TEXT("Unversioned -> current"), Unversioned->Version, ULinguaSpySaveGame::CurrentVersion);

	// Round trip: a current profile must come back as current (Version is written because it differs from 0)
	TArray<uint8> Bytes;
	UGameplayStatics::SaveGameToMemory(Unversioned, Bytes);
	const ULinguaSpySaveGame* Loaded = Cast<ULinguaSpySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestTrue(TEXT("Version survives serialization"), Loaded && Loaded->Version == ULinguaSpySaveGame::CurrentVersion);
	TestFalse(TEXT("Null is safe"), ULinguaSpySaveGame::Migrate(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpyFluencyGradeTest, "LinguaSpy.FluencyGrade", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpyFluencyGradeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Clean run = S"), UFluencyScoring::ComputeGrade(6, 0, 0), 0);
	TestEqual(TEXT("No challenges, no help = S"), UFluencyScoring::ComputeGrade(0, 0, 0), 0);
	TestEqual(TEXT("One peek in 6 = A"), UFluencyScoring::ComputeGrade(6, 0, 1), 1);
	TestEqual(TEXT("One mistake in 6 = A"), UFluencyScoring::ComputeGrade(6, 1, 0), 1);
	TestEqual(TEXT("Two mistakes in 6 = B"), UFluencyScoring::ComputeGrade(6, 2, 0), 2);
	TestEqual(TEXT("Mistakes + peeks in 6 = B"), UFluencyScoring::ComputeGrade(6, 2, 3), 2);
	TestEqual(TEXT("Heavy reliance = C"), UFluencyScoring::ComputeGrade(6, 3, 6), 3);
	TestEqual(TEXT("Peeking every line = C"), UFluencyScoring::ComputeGrade(3, 0, 8), 3);
	TestEqual(TEXT("Letter"), UFluencyScoring::GradeLetter(1).ToString(), FString(TEXT("A")));
	TestEqual(TEXT("Letter clamps"), UFluencyScoring::GradeLetter(9).ToString(), FString(TEXT("C")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLinguaSpySaveSrsRoundTripTest, "LinguaSpy.SaveRoundTripSrs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLinguaSpySaveSrsRoundTripTest::RunTest(const FString& Parameters)
{
	ULinguaSpySaveGame* Save = NewObject<ULinguaSpySaveGame>();
	FSrsItemState Item;
	Item.Box = 3;
	Item.DueUtcTicks = FDateTime(2026, 11, 2).GetTicks();
	Item.CityId = TEXT("Tokyo");
	Item.SkillTag = TEXT("Shopping");
	Item.AssistedPasses = 2;
	Save->SrsItems.Add(TEXT("/Game/X.X#Node#-1"), Item);
	FMissionGrade Grade;
	Grade.Grade = 1;
	Grade.Peeks = 2;
	Save->BestGrades.Add(TEXT("Tokyo_Konbini"), Grade);
	Save->Settings.MusicVolume = 0.25f;

	TArray<uint8> Bytes;
	TestTrue(TEXT("Serialize"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
	const ULinguaSpySaveGame* Loaded = Cast<ULinguaSpySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Deserialize"), Loaded))
	{
		return false;
	}
	const FSrsItemState* LoadedItem = Loaded->SrsItems.Find(TEXT("/Game/X.X#Node#-1"));
	if (TestNotNull(TEXT("SRS item"), LoadedItem))
	{
		TestEqual(TEXT("Box"), LoadedItem->Box, 3);
		TestEqual(TEXT("Due"), LoadedItem->DueUtcTicks, Item.DueUtcTicks);
		TestEqual(TEXT("Skill"), LoadedItem->SkillTag, FName(TEXT("Shopping")));
		TestEqual(TEXT("Assisted"), LoadedItem->AssistedPasses, 2);
	}
	TestEqual(TEXT("Best grade"), Loaded->BestGrades.FindRef(TEXT("Tokyo_Konbini")).Grade, 1);
	TestEqual(TEXT("Music volume"), Loaded->Settings.MusicVolume, 0.25f);
	return true;
}

#endif
