// LinguaSpy - per-skill learning progress

#include "LanguageProgressSubsystem.h"
#include "LinguaSpySaveSubsystem.h"

void ULanguageProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = Collection.InitializeDependency<ULinguaSpySaveSubsystem>();
}

FName ULanguageProgressSubsystem::MakeKey(FName CityId, FName SkillTag)
{
	return FName(*FString::Printf(TEXT("%s.%s"), *CityId.ToString(), *SkillTag.ToString()));
}

TMap<FName, FSkillStats>* ULanguageProgressSubsystem::GetStatsMap() const
{
	ULinguaSpySaveGame* Save = SaveSubsystem ? SaveSubsystem->GetSave() : nullptr;
	return Save ? &Save->SkillStats : nullptr;
}

void ULanguageProgressSubsystem::RecordResult(FName CityId, FName SkillTag, bool bPassed)
{
	if (TMap<FName, FSkillStats>* Stats = GetStatsMap())
	{
		FSkillStats& Entry = Stats->FindOrAdd(MakeKey(CityId, SkillTag));
		if (bPassed)
		{
			++Entry.Passes;
		}
		else
		{
			++Entry.Failures;
		}
		// Saved together with the next mission/conversation checkpoint
	}
}

FSkillStats ULanguageProgressSubsystem::GetStats(FName CityId, FName SkillTag) const
{
	const TMap<FName, FSkillStats>* Stats = GetStatsMap();
	const FSkillStats* Entry = Stats ? Stats->Find(MakeKey(CityId, SkillTag)) : nullptr;
	return Entry ? *Entry : FSkillStats();
}

TArray<FName> ULanguageProgressSubsystem::GetWeakSkills(FName CityId) const
{
	TArray<TPair<FName, int32>> Weak;
	const TMap<FName, FSkillStats>* Stats = GetStatsMap();
	if (!Stats)
	{
		return {};
	}

	const FString Prefix = CityId.ToString() + TEXT(".");
	for (const TPair<FName, FSkillStats>& Pair : *Stats)
	{
		const FString Key = Pair.Key.ToString();
		const int32 Deficit = Pair.Value.Failures - Pair.Value.Passes;
		if (Key.StartsWith(Prefix) && Deficit > 0)
		{
			Weak.Emplace(FName(*Key.RightChop(Prefix.Len())), Deficit);
		}
	}
	Weak.Sort([](const TPair<FName, int32>& A, const TPair<FName, int32>& B) { return A.Value > B.Value; });

	TArray<FName> Out;
	for (const TPair<FName, int32>& Pair : Weak)
	{
		Out.Add(Pair.Key);
	}
	return Out;
}
