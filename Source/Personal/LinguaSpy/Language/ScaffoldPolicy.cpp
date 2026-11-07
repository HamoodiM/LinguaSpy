// LinguaSpy - scaffold ladder: how much support a challenge gets (docs/Learning_Design.md sections 2-3)

#include "ScaffoldPolicy.h"
#include "SentenceGrader.h"
#include "LinguaSpySaveGame.h"
#include "Math/RandomStream.h"

#define LOCTEXT_NAMESPACE "LinguaSpyScaffold"

EScaffoldLevel UScaffoldPolicy::ChooseLevel(const FSrsItemState* State, int32 SessionErrors, bool bTypingAllowed)
{
	int32 Level = static_cast<int32>(EScaffoldLevel::Assemble);
	const int32 Box = State ? State->Box : 0;
	if (Box >= 5)
	{
		Level = static_cast<int32>(EScaffoldLevel::Produce);
	}
	else if (Box >= 3)
	{
		Level = static_cast<int32>(EScaffoldLevel::GapFill);
	}
	else if (Box >= 1)
	{
		Level = static_cast<int32>(EScaffoldLevel::AssemblePlus);
	}

	if (!bTypingAllowed)
	{
		Level = FMath::Min(Level, static_cast<int32>(EScaffoldLevel::AssemblePlus));
	}
	Level = FMath::Max(Level - FMath::Max(SessionErrors, 0), static_cast<int32>(EScaffoldLevel::Recognize));
	return static_cast<EScaffoldLevel>(Level);
}

TArray<FWordTile> UScaffoldPolicy::BuildTiles(const FWordTileChallenge& Challenge, EScaffoldLevel Level)
{
	if (Level != EScaffoldLevel::Assemble || Challenge.AcceptedAnswers.IsEmpty())
	{
		return Challenge.Tiles;
	}

	// Only the tiles the first answer needs (as a multiset, so repeated words keep their copies)
	TArray<FString> Needed;
	for (const FString& Token : Challenge.AcceptedAnswers[0].Tokens)
	{
		Needed.Add(USentenceGrader::NormalizeToken(Token));
	}
	TArray<FWordTile> Out;
	for (const FWordTile& Tile : Challenge.Tiles)
	{
		if (Needed.RemoveSingle(USentenceGrader::NormalizeToken(Tile.Text)) > 0)
		{
			Out.Add(Tile);
		}
	}
	return Out.IsEmpty() ? Challenge.Tiles : Out;
}

TArray<int32> UScaffoldPolicy::GetGapIndices(const FWordTileChallenge& Challenge)
{
	TArray<int32> Gaps;
	const int32 Len = Challenge.AcceptedAnswers.IsEmpty() ? 0 : Challenge.AcceptedAnswers[0].Tokens.Num();
	for (const int32 Gap : Challenge.GapTokenIndices)
	{
		if (Gap >= 0 && Gap < Len)
		{
			Gaps.AddUnique(Gap);
		}
	}
	if (!Gaps.IsEmpty() || Len == 0)
	{
		Gaps.Sort();
		return Gaps;
	}

	// Automatic: the longest words carry the most meaning (ties -> later word, usually the content word)
	TArray<int32> Order;
	for (int32 i = 0; i < Len; ++i)
	{
		Order.Add(i);
	}
	const TArray<FString>& Tokens = Challenge.AcceptedAnswers[0].Tokens;
	Order.StableSort([&Tokens](int32 A, int32 B)
	{
		const int32 LenA = Tokens[A].Len();
		const int32 LenB = Tokens[B].Len();
		return LenA != LenB ? LenA > LenB : A > B;
	});
	const int32 Count = Len >= 6 ? 2 : 1;
	for (int32 i = 0; i < Count && i < Order.Num(); ++i)
	{
		Gaps.Add(Order[i]);
	}
	Gaps.Sort();
	return Gaps;
}

FString UScaffoldPolicy::BuildGapDisplay(const FWordTileChallenge& Challenge, bool bUseSpaces)
{
	if (Challenge.AcceptedAnswers.IsEmpty())
	{
		return FString();
	}
	TArray<FString> Tokens = Challenge.AcceptedAnswers[0].Tokens;
	for (const int32 Gap : GetGapIndices(Challenge))
	{
		Tokens[Gap] = TEXT("____");
	}
	return USentenceGrader::JoinTokens(Tokens, bUseSpaces);
}

TArray<FString> UScaffoldPolicy::FillGaps(const FWordTileChallenge& Challenge, const TArray<FString>& TypedGapWords)
{
	if (Challenge.AcceptedAnswers.IsEmpty())
	{
		return TypedGapWords;
	}
	TArray<FString> Tokens = Challenge.AcceptedAnswers[0].Tokens;
	const TArray<int32> Gaps = GetGapIndices(Challenge);
	for (int32 i = 0; i < Gaps.Num(); ++i)
	{
		Tokens[Gaps[i]] = TypedGapWords.IsValidIndex(i) ? TypedGapWords[i].TrimStartAndEnd() : FString();
	}
	return Tokens;
}

void UScaffoldPolicy::BuildRecognitionOptions(const FWordTileChallenge& Challenge, bool bUseSpaces, int32 Seed,
	TArray<FString>& OutOptions, int32& OutCorrectIndex)
{
	OutOptions.Reset();
	OutCorrectIndex = INDEX_NONE;
	if (Challenge.AcceptedAnswers.IsEmpty())
	{
		return;
	}

	const TArray<FString>& Model = Challenge.AcceptedAnswers[0].Tokens;
	const FString Correct = USentenceGrader::JoinTokens(Model, bUseSpaces);

	// A wrong option must not accidentally be an accepted answer
	auto IsAccepted = [&Challenge](const TArray<FString>& Tokens)
	{
		return USentenceGrader::Grade(Challenge, Tokens).bPassed;
	};

	TArray<FString> Wrong;
	for (const FString& Authored : Challenge.RecognitionDistractors)
	{
		if (!Authored.IsEmpty() && Authored != Correct && Wrong.Num() < 2)
		{
			Wrong.AddUnique(Authored);
		}
	}

	FRandomStream Random(Seed);

	// Swap one distractor tile into the answer: "je voudrais un thé" for "je voudrais un café"
	if (Wrong.Num() < 2)
	{
		TArray<FString> NeededNorm;
		for (const FString& Token : Model)
		{
			NeededNorm.Add(USentenceGrader::NormalizeToken(Token));
		}
		TArray<FString> Distractors;
		for (const FWordTile& Tile : Challenge.Tiles)
		{
			if (NeededNorm.RemoveSingle(USentenceGrader::NormalizeToken(Tile.Text)) == 0)
			{
				Distractors.Add(Tile.Text);
			}
		}
		for (int32 d = 0; d < Distractors.Num() && Wrong.Num() < 2; ++d)
		{
			// Try each position, starting from a seeded one, until the result is not an accepted answer
			const int32 Start = Random.RandRange(0, Model.Num() - 1);
			for (int32 Offset = 0; Offset < Model.Num(); ++Offset)
			{
				TArray<FString> Candidate = Model;
				Candidate[(Start + Offset) % Model.Num()] = Distractors[d];
				const FString Joined = USentenceGrader::JoinTokens(Candidate, bUseSpaces);
				if (Joined != Correct && !Wrong.Contains(Joined) && !IsAccepted(Candidate))
				{
					Wrong.Add(Joined);
					break;
				}
			}
		}
	}

	// Last resort: wrong word order
	for (int32 i = 0; i + 1 < Model.Num() && Wrong.Num() < 2; ++i)
	{
		TArray<FString> Candidate = Model;
		Candidate.Swap(i, i + 1);
		const FString Joined = USentenceGrader::JoinTokens(Candidate, bUseSpaces);
		if (Joined != Correct && !Wrong.Contains(Joined) && !IsAccepted(Candidate))
		{
			Wrong.Add(Joined);
		}
	}

	OutOptions = Wrong;
	OutOptions.Add(Correct);
	for (int32 i = OutOptions.Num() - 1; i > 0; --i)
	{
		OutOptions.Swap(i, Random.RandRange(0, i));
	}
	OutCorrectIndex = OutOptions.IndexOfByKey(Correct);
}

bool UScaffoldPolicy::IsAssistedPass(EScaffoldLevel Level, int32 Attempt)
{
	return Level == EScaffoldLevel::Recognize || Attempt > 1;
}

FText UScaffoldPolicy::ChooseHint(const FWordTileChallenge& Challenge, const FGradeResult& Result, int32 SubmittedTokens,
	EScaffoldLevel Level, bool bUsesWordSpacing)
{
	const bool bRecognize = Level == EScaffoldLevel::Recognize;
	if (Result.bAccentOnly && !bRecognize)
	{
		return LOCTEXT("AccentHint", "So close - the words are right. Check the accents.");
	}
	for (const FChallengeHint& Hint : Challenge.Hints)
	{
		if (!bRecognize && Hint.TokenIndex >= 0 && Hint.TokenIndex == Result.FirstWrongIndex && !Hint.Text.IsEmpty())
		{
			return Hint.Text;
		}
	}
	for (const FChallengeHint& Hint : Challenge.Hints)
	{
		if (Hint.TokenIndex < 0 && !Hint.Text.IsEmpty())
		{
			return Hint.Text;
		}
	}
	if (bRecognize)
	{
		return LOCTEXT("RecognizeHint", "Not quite - that sentence says something else. Read the line again and pick the one that answers it.");
	}
	const int32 Expected = Challenge.AcceptedAnswers.IsEmpty() ? 0 : Challenge.AcceptedAnswers[0].Tokens.Num();
	if (Expected > 0 && SubmittedTokens != Expected && Result.ClosestAnswerIndex == 0)
	{
		return bUsesWordSpacing
			? FText::Format(LOCTEXT("LengthHint", "Not quite. The phrase has {0} {0}|plural(one=word,other=words)."), Expected)
			: FText::Format(LOCTEXT("LengthHintTiles", "Not quite. The phrase is built from {0} {0}|plural(one=tile,other=tiles)."), Expected);
	}
	return bUsesWordSpacing
		? LOCTEXT("GenericHint", "Not quite. Look at the highlighted word and try again.")
		: LOCTEXT("GenericHintTiles", "Not quite. Look at the highlighted tile and try again.");
}

FString UScaffoldPolicy::LevelName(EScaffoldLevel Level)
{
	switch (Level)
	{
	case EScaffoldLevel::Recognize:    return TEXT("Recognize");
	case EScaffoldLevel::Assemble:     return TEXT("Assemble");
	case EScaffoldLevel::AssemblePlus: return TEXT("AssemblePlus");
	case EScaffoldLevel::GapFill:      return TEXT("GapFill");
	case EScaffoldLevel::Produce:      return TEXT("Produce");
	}
	return TEXT("Unknown");
}

#undef LOCTEXT_NAMESPACE
