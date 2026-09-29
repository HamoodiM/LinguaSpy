// LinguaSpy - spaced-repetition scheduling (Leitner boxes on real time)

#include "SrsScheduler.h"

FString FSrsItemKey::ToString() const
{
	return FString::Printf(TEXT("%s#%s#%d"), *Conversation.ToString(), *NodeId.ToString(), ChoiceIndex);
}

bool FSrsItemKey::Parse(const FString& Key, FSrsItemKey& OutKey)
{
	FString Rest;
	FString ChoiceString;
	FString Path;
	FString Node;
	if (!Key.Split(TEXT("#"), &Rest, &ChoiceString, ESearchCase::CaseSensitive, ESearchDir::FromEnd)
		|| !Rest.Split(TEXT("#"), &Path, &Node, ESearchCase::CaseSensitive, ESearchDir::FromEnd)
		|| Path.IsEmpty() || Node.IsEmpty() || ChoiceString.IsEmpty())
	{
		return false;
	}
	// IsNumeric accepts a leading '-', so INDEX_NONE (-1) round-trips
	if (!ChoiceString.IsNumeric())
	{
		return false;
	}
	OutKey.Conversation = FSoftObjectPath(Path);
	OutKey.NodeId = FName(*Node);
	OutKey.ChoiceIndex = FCString::Atoi(*ChoiceString);
	return OutKey.IsValid();
}

FTimespan USrsScheduler::GetInterval(int32 Box)
{
	switch (FMath::Clamp(Box, 0, MaxBox))
	{
	case 0:
	case 1:  return FTimespan::FromMinutes(10.0);
	case 2:  return FTimespan::FromDays(1.0);
	case 3:  return FTimespan::FromDays(3.0);
	case 4:  return FTimespan::FromDays(7.0);
	default: return FTimespan::FromDays(21.0);
	}
}

void USrsScheduler::Update(FSrsItemState& State, bool bPassed, bool bAssisted, const FDateTime& Now)
{
	if (!bPassed)
	{
		++State.Failures;
		State.Box = 1;
	}
	else if (bAssisted)
	{
		++State.Passes;
		++State.AssistedPasses;
	}
	else
	{
		++State.Passes;
		State.Box = FMath::Min(State.Box + 1, MaxBox);
	}
	State.LastReviewUtcTicks = Now.GetTicks();
	State.DueUtcTicks = (Now + GetInterval(State.Box)).GetTicks();
}

bool USrsScheduler::IsDue(const FSrsItemState& State, const FDateTime& Now)
{
	return State.DueUtcTicks <= Now.GetTicks();
}

float USrsScheduler::GetStrength(const FSrsItemState& State)
{
	return FMath::Clamp(static_cast<float>(State.Box) / MaxBox, 0.f, 1.f);
}

TArray<FString> USrsScheduler::SelectReviewItems(const TMap<FString, FSrsItemState>& Items, FName CityId, const FDateTime& Now, int32 Max, bool bFillWithNotDue)
{
	using FEntry = TPair<FString, const FSrsItemState*>;
	TArray<FEntry> Due;
	TArray<FEntry> NotDue;
	for (const TPair<FString, FSrsItemState>& Pair : Items)
	{
		if (!CityId.IsNone() && Pair.Value.CityId != CityId)
		{
			continue;
		}
		(IsDue(Pair.Value, Now) ? Due : NotDue).Emplace(Pair.Key, &Pair.Value);
	}

	// Most overdue first, then weakest; key as a final tie-break so the order is deterministic
	Due.Sort([](const FEntry& A, const FEntry& B)
	{
		if (A.Value->DueUtcTicks != B.Value->DueUtcTicks) { return A.Value->DueUtcTicks < B.Value->DueUtcTicks; }
		if (A.Value->Box != B.Value->Box) { return A.Value->Box < B.Value->Box; }
		return A.Key < B.Key;
	});
	// Weakest first: lowest box, most failures, soonest due
	NotDue.Sort([](const FEntry& A, const FEntry& B)
	{
		if (A.Value->Box != B.Value->Box) { return A.Value->Box < B.Value->Box; }
		if (A.Value->Failures != B.Value->Failures) { return A.Value->Failures > B.Value->Failures; }
		if (A.Value->DueUtcTicks != B.Value->DueUtcTicks) { return A.Value->DueUtcTicks < B.Value->DueUtcTicks; }
		return A.Key < B.Key;
	});

	TArray<FString> Out;
	for (const FEntry& Entry : Due)
	{
		if (Out.Num() >= Max)
		{
			return Out;
		}
		Out.Add(Entry.Key);
	}
	if (bFillWithNotDue)
	{
		for (const FEntry& Entry : NotDue)
		{
			if (Out.Num() >= Max)
			{
				break;
			}
			Out.Add(Entry.Key);
		}
	}
	return Out;
}

int32 USrsScheduler::CountDue(const TMap<FString, FSrsItemState>& Items, FName CityId, const FDateTime& Now)
{
	int32 Count = 0;
	for (const TPair<FString, FSrsItemState>& Pair : Items)
	{
		if ((CityId.IsNone() || Pair.Value.CityId == CityId) && IsDue(Pair.Value, Now))
		{
			++Count;
		}
	}
	return Count;
}
