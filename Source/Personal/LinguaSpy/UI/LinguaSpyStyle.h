// LinguaSpy - shared colours/fonts for the code-built default UI (stylized, flat, pastel)

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Components/TextBlock.h"

namespace LinguaSpyStyle
{
	inline const FLinearColor PanelBg      = FLinearColor::FromSRGBColor(FColor(0x22, 0x26, 0x3A, 0xF0));
	inline const FLinearColor PanelInset   = FLinearColor::FromSRGBColor(FColor(0x2F, 0x35, 0x50, 0xFF));
	inline const FLinearColor TextMain     = FLinearColor::FromSRGBColor(FColor(0xF6, 0xF1, 0xE7));
	inline const FLinearColor TextHelper   = FLinearColor::FromSRGBColor(FColor(0x9F, 0xD8, 0xCB));
	inline const FLinearColor TextMuted    = FLinearColor::FromSRGBColor(FColor(0xB8, 0xB4, 0xC8));
	inline const FLinearColor Accent       = FLinearColor::FromSRGBColor(FColor(0xF2, 0xC1, 0x4E));
	inline const FLinearColor TileNeutral  = FLinearColor::FromSRGBColor(FColor(0xF6, 0xF1, 0xE7));
	inline const FLinearColor TileCorrect  = FLinearColor::FromSRGBColor(FColor(0x8F, 0xD6, 0x94));
	inline const FLinearColor TileWrong    = FLinearColor::FromSRGBColor(FColor(0xF2, 0x8B, 0x82));
	inline const FLinearColor TileText     = FLinearColor::FromSRGBColor(FColor(0x22, 0x26, 0x3A));
	inline const FLinearColor Danger       = FLinearColor::FromSRGBColor(FColor(0xE0, 0x5A, 0x4F));

	/** Applies size / colour to a text block, keeping the default (Roboto) font family which covers accents. */
	inline void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color, bool bBold = false)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
	}
}
