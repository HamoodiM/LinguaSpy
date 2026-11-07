// LinguaSpy - persistent player profile

#include "LinguaSpySaveGame.h"

bool ULinguaSpySaveGame::Migrate(ULinguaSpySaveGame* Save)
{
	if (!Save || Save->Version >= CurrentVersion)
	{
		return false;
	}

	// v1 (stored as 0: the old default was never serialized) -> v2: SrsItems / BestGrades start empty (filled as the player answers), volume settings take their defaults,
	// and the old global "helper on" toggle is replaced by per-line tap-to-translate.
	if (Save->Version < 2)
	{
		Save->Settings.bAlwaysShowTranslation = false;
		Save->Version = 2;
	}

	// v2 -> v3: players who already played a city keep tap-to-translate (OnCall), or Shadowing if they had
	// "always show translations" on; finished missions count as briefed (no surprise briefing for old content).
	if (Save->Version < 3)
	{
		const EHandlerMode Mode = Save->Settings.bAlwaysShowTranslation ? EHandlerMode::Shadowing : EHandlerMode::OnCall;
		for (const TPair<FName, FCityProgress>& City : Save->Cities)
		{
			Save->HandlerModes.Add(City.Key, Mode);
			for (const FName Mission : City.Value.CompletedMissions)
			{
				Save->BriefingsCompleted.AddUnique(Mission);
			}
		}
		Save->Settings.bAlwaysShowTranslation = false;
		Save->Version = 3;
	}

	// v3 -> v4: FMissionGrade gained Retries. Older best grades stored retries inside Peeks and can't be split; their
	// grade letter (which used the sum) stays correct, so nothing is converted.
	if (Save->Version < 4)
	{
		Save->Version = 4;
	}

	Save->Version = CurrentVersion;
	return true;
}
