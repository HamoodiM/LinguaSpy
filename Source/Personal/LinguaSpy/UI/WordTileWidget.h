// LinguaSpy - a single clickable word tile

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WordTileWidget.generated.h"

class UButton;
class UTextBlock;
class UWordTileWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWordTileClicked, UWordTileWidget*, Tile);

UENUM(BlueprintType)
enum class EWordTileState : uint8
{
	Neutral,
	Correct,
	Wrong
};

/**
 *  A word tile. Builds a default button+text layout in C++ when used without a Blueprint layout;
 *  a WBP subclass may provide its own widgets named TileButton / TileText.
 */
UCLASS()
class UWordTileWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Tile")
	FOnWordTileClicked OnTileClicked;

	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetWord(const FString& InWord);

	UFUNCTION(BlueprintPure, Category="Tile")
	const FString& GetWord() const { return Word; }

	/** Optional small second line (e.g. romaji). Empty hides it. */
	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetReading(const FString& InReading);

	/** Smaller, wider variant used for choice buttons. */
	void SetChoiceStyle();

	/** Uses a language's font for the word (e.g. a CJK font). Null keeps the default. */
	void SetNativeFont(class UFont* Font);

	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetTileState(EWordTileState NewState);

	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetLocked(bool bLocked);

	/** Little scale pop (on spawn / when moved). */
	void PlayPop();

	/** Side-to-side shake (wrong answer). */
	void PlayShake();

	/** True while the tile sits in the answer row rather than the bank. */
	bool bInAnswer = false;

protected:

	UPROPERTY(BlueprintReadOnly, Category="Tile", meta=(BindWidgetOptional))
	TObjectPtr<UButton> TileButton;

	UPROPERTY(BlueprintReadOnly, Category="Tile", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TileText;

	UPROPERTY(BlueprintReadOnly, Category="Tile", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadingText;

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UFUNCTION()
	void HandleClicked();

	FString Word;
	float PopTime = -1.f;
	float ShakeTime = -1.f;
};
