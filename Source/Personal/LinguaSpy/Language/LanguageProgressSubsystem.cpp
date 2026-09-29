// LinguaSpy - per-skill learning progress

#include "LanguageProgressSubsystem.h"

void ULanguageProgressSubsystem::RecordResult(FName SkillTag, bool bPassed)
{
	FSkillStats& Entry = Stats.FindOrAdd(SkillTag);
	if (bPassed)
	{
		++Entry.Passes;
	}
	else
	{
		++Entry.Failures;
	}
}

FSkillStats ULanguageProgressSubsystem::GetStats(FName SkillTag) const
{
	const FSkillStats* Entry = Stats.Find(SkillTag);
	return Entry ? *Entry : FSkillStats();
}

TArray<FName> ULanguageProgressSubsystem::GetWeakSkills() const
{
	TArray<FName> Weak;
	for (const TPair<FName, FSkillStats>& Pair : Stats)
	{
		if (Pair.Value.Failures > Pair.Value.Passes)
		{
			Weak.Add(Pair.Key);
		}
	}
	Weak.Sort([this](const FName& A, const FName& B)
	{
		const FSkillStats& SA = Stats[A];
		const FSkillStats& SB = Stats[B];
		return (SA.Failures - SA.Passes) > (SB.Failures - SB.Passes);
	});
	return Weak;
}
