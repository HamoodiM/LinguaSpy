// LinguaSpy - word-tile answer grading

#include "SentenceGrader.h"

namespace
{
	bool IsStrippablePunctuation(TCHAR C)
	{
		switch (C)
		{
		case TEXT(','): case TEXT('.'): case TEXT('!'): case TEXT('?'):
		case TEXT(';'): case TEXT(':'): case TEXT('"'):
		case 0x00AB: /* « */ case 0x00BB: /* » */ case 0x00BF: /* ¿ */ case 0x00A1: /* ¡ */
			return true;
		default:
			return FChar::IsWhitespace(C);
		}
	}
}

FString USentenceGrader::NormalizeToken(const FString& Token)
{
	int32 Start = 0;
	int32 End = Token.Len();
	while (Start < End && IsStrippablePunctuation(Token[Start])) { ++Start; }
	while (End > Start && IsStrippablePunctuation(Token[End - 1])) { --End; }

	// FText::ToLower is Unicode-aware (FString::ToLower only folds ASCII, so "CAFÉ" would stay "cafÉ")
	FString Result = FText::FromString(Token.Mid(Start, End - Start)).ToLower().ToString();
	// Treat typographic apostrophes the same as straight ones
	Result.ReplaceCharInline(0x2019, TEXT('\''));
	Result.ReplaceCharInline(0x2018, TEXT('\''));
	return Result;
}

FGradeResult USentenceGrader::Grade(const FWordTileChallenge& Challenge, const TArray<FString>& PlayerTokens)
{
	FGradeResult Result;
	Result.TileCorrect.Init(false, PlayerTokens.Num());

	TArray<FString> Player;
	Player.Reserve(PlayerTokens.Num());
	for (const FString& Token : PlayerTokens)
	{
		Player.Add(NormalizeToken(Token));
	}

	int32 BestScore = -1;
	for (int32 AnswerIndex = 0; AnswerIndex < Challenge.AcceptedAnswers.Num(); ++AnswerIndex)
	{
		const TArray<FString>& Answer = Challenge.AcceptedAnswers[AnswerIndex].Tokens;

		TArray<bool> Correct;
		Correct.Init(false, Player.Num());
		int32 Score = 0;
		for (int32 i = 0; i < Player.Num(); ++i)
		{
			if (Answer.IsValidIndex(i) && Player[i] == NormalizeToken(Answer[i]))
			{
				Correct[i] = true;
				++Score;
			}
		}

		const bool bExact = Score == Answer.Num() && Player.Num() == Answer.Num() && Answer.Num() > 0;
		// Penalize length mismatch so the closest answer is also the closest in size
		Score -= FMath::Abs(Answer.Num() - Player.Num());

		if (bExact || Score > BestScore)
		{
			BestScore = Score;
			Result.ClosestAnswerIndex = AnswerIndex;
			Result.TileCorrect = MoveTemp(Correct);
			Result.bPassed = bExact;
		}
		if (bExact)
		{
			break;
		}
	}

	return Result;
}

FString USentenceGrader::JoinTokens(const TArray<FString>& Tokens)
{
	FString Out;
	for (const FString& Token : Tokens)
	{
		// French-style: no space after an elided apostrophe (e.g. "s'" + "il")
		if (!Out.IsEmpty() && !Out.EndsWith(TEXT("'")) && !Out.EndsWith(TEXT("’")))
		{
			Out += TEXT(" ");
		}
		Out += Token;
	}
	return Out;
}
