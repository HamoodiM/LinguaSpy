// LinguaSpy - fluency grade from helper reliance and mistakes

#include "FluencyScoring.h"

float UFluencyScoring::ComputeReliance(int32 Challenges, int32 Mistakes, int32 Peeks)
{
	const float Weighted = FMath::Max(Mistakes, 0) + PeekWeight * FMath::Max(Peeks, 0);
	return Weighted / FMath::Max(Challenges, 1);
}

int32 UFluencyScoring::ComputeGrade(int32 Challenges, int32 Mistakes, int32 Peeks)
{
	if (Mistakes <= 0 && Peeks <= 0)
	{
		return 0;
	}
	const float Reliance = ComputeReliance(Challenges, Mistakes, Peeks);
	return Reliance <= 0.25f ? 1 : Reliance <= 0.6f ? 2 : 3;
}

FText UFluencyScoring::GradeLetter(int32 Grade)
{
	static const TCHAR* Letters[] = { TEXT("S"), TEXT("A"), TEXT("B"), TEXT("C") };
	return FText::FromString(Letters[FMath::Clamp(Grade, 0, 3)]);
}
