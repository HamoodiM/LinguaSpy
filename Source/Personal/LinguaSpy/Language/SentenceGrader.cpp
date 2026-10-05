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
		// Japanese / full-width punctuation
		case 0x3002: /* 。 */ case 0x3001: /* 、 */ case 0xFF01: /* ！ */ case 0xFF1F: /* ？ */
		case 0x300C: /* 「 */ case 0x300D: /* 」 */ case 0x3000: /* full-width space */ case 0xFF0C: /* ， */
		case 0x2026: /* … */
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

FString USentenceGrader::FoldAccents(const FString& Text)
{
	// Latin-1 Supplement + Latin Extended-A letters used by French / Spanish / Italian (escaped: source stays ASCII)
	struct FFold { TCHAR From; char To; };
	static const FFold Table[] = { {0x00E0, 'a'}, {0x00E1, 'a'}, {0x00E2, 'a'}, {0x00E3, 'a'}, {0x00E4, 'a'}, {0x00E5, 'a'}, {0x00E8, 'e'}, {0x00E9, 'e'}, {0x00EA, 'e'}, {0x00EB, 'e'}, {0x00EC, 'i'}, {0x00ED, 'i'}, {0x00EE, 'i'}, {0x00EF, 'i'}, {0x00F2, 'o'}, {0x00F3, 'o'}, {0x00F4, 'o'}, {0x00F5, 'o'}, {0x00F6, 'o'}, {0x00F9, 'u'}, {0x00FA, 'u'}, {0x00FB, 'u'}, {0x00FC, 'u'}, {0x00FD, 'y'}, {0x00FF, 'y'}, {0x00F1, 'n'}, {0x00E7, 'c'}, {0x0153, 'o'}, {0x00E6, 'a'}, {0x00C0, 'A'}, {0x00C1, 'A'}, {0x00C2, 'A'}, {0x00C3, 'A'}, {0x00C4, 'A'}, {0x00C5, 'A'}, {0x00C8, 'E'}, {0x00C9, 'E'}, {0x00CA, 'E'}, {0x00CB, 'E'}, {0x00CC, 'I'}, {0x00CD, 'I'}, {0x00CE, 'I'}, {0x00CF, 'I'}, {0x00D2, 'O'}, {0x00D3, 'O'}, {0x00D4, 'O'}, {0x00D5, 'O'}, {0x00D6, 'O'}, {0x00D9, 'U'}, {0x00DA, 'U'}, {0x00DB, 'U'}, {0x00DC, 'U'}, {0x00DD, 'Y'}, {0x00D1, 'N'}, {0x00C7, 'C'} };
	FString Result = Text;
	for (TCHAR& C : Result)
	{
		if (C < 0x00C0)
		{
			continue;
		}
		for (const FFold& Entry : Table)
		{
			if (Entry.From == C)
			{
				C = TCHAR(Entry.To);
				break;
			}
		}
	}
	return Result;
}

TArray<FString> USentenceGrader::TokenizeTyped(const FString& Text)
{
	TArray<FString> Tokens;
	FString Current;
	for (const TCHAR C : Text)
	{
		if (FChar::IsWhitespace(C) || C == 0x3000)
		{
			if (!Current.IsEmpty())
			{
				Tokens.Add(Current);
				Current.Reset();
			}
		}
		else
		{
			Current.AppendChar(C);
		}
	}
	if (!Current.IsEmpty())
	{
		Tokens.Add(Current);
	}
	// Drop tokens that are only punctuation ("Bonjour !" -> "Bonjour")
	Tokens.RemoveAll([](const FString& Token) { return NormalizeToken(Token).IsEmpty(); });
	return Tokens;
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

	if (!Result.bPassed)
	{
		Result.FirstWrongIndex = Result.TileCorrect.Find(false);
		if (Result.FirstWrongIndex == INDEX_NONE)
		{
			// Every placed token is right but something is missing
			Result.FirstWrongIndex = Player.Num();
		}

		// Would it pass without accents? Then the learner knows the words and only needs the accent hint.
		for (const FAcceptedAnswer& Answer : Challenge.AcceptedAnswers)
		{
			if (Answer.Tokens.Num() != Player.Num() || Answer.Tokens.IsEmpty())
			{
				continue;
			}
			bool bMatch = true;
			for (int32 i = 0; i < Player.Num() && bMatch; ++i)
			{
				bMatch = FoldAccents(Player[i]) == FoldAccents(NormalizeToken(Answer.Tokens[i]));
			}
			if (bMatch)
			{
				Result.bAccentOnly = true;
				break;
			}
		}
	}

	return Result;
}

FString USentenceGrader::JoinTokens(const TArray<FString>& Tokens, bool bUseSpaces)
{
	// Convention: elided words are single tiles ("s'il", "d'acqua"), so joining is just spaces (or nothing)
	return FString::Join(Tokens, bUseSpaces ? TEXT(" ") : TEXT(""));
}
