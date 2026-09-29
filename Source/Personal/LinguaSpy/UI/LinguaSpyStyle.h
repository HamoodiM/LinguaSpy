// LinguaSpy - shared colours/fonts/brushes for the code-built UI (stylized, flat, pastel, rounded)

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Components/Border.h"
#include "Components/Button.h"
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

	inline constexpr float PanelRadius  = 14.f;
	inline constexpr float ButtonRadius = 10.f;
	inline constexpr float CardRadius   = 18.f;

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

	/** Rounded rectangle brush with optional outline. */
	inline FSlateBrush RoundedBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}

	/** White rounded button style; tint it with UButton::SetBackgroundColor. */
	inline FButtonStyle RoundedButtonStyle(float Radius = ButtonRadius)
	{
		FButtonStyle Style;
		Style.SetNormal(RoundedBrush(FLinearColor::White, Radius));
		Style.SetHovered(RoundedBrush(FLinearColor(0.9f, 0.9f, 0.9f), Radius, FLinearColor::White, 2.f));
		Style.SetPressed(RoundedBrush(FLinearColor(0.75f, 0.75f, 0.75f), Radius));
		Style.SetDisabled(RoundedBrush(FLinearColor(0.55f, 0.55f, 0.55f, 0.6f), Radius));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	inline void StyleButton(UButton* Button, float Radius = ButtonRadius)
	{
		if (Button)
		{
			Button->SetStyle(RoundedButtonStyle(Radius));
		}
	}

	/** Rounded panel background. */
	inline void StyleBorder(UBorder* Border, const FLinearColor& Color, float Radius = PanelRadius)
	{
		if (Border)
		{
			Border->SetBrush(RoundedBrush(FLinearColor::White, Radius));
			Border->SetBrushColor(Color);
		}
	}

	/** Ease-out with a little overshoot, for pop-in animations (T in 0..1). */
	inline float EaseOutBack(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		constexpr float C1 = 1.70158f;
		constexpr float C3 = C1 + 1.f;
		return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	}
}
