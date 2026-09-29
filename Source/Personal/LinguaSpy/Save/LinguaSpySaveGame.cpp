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

	Save->Version = CurrentVersion;
	return true;
}
