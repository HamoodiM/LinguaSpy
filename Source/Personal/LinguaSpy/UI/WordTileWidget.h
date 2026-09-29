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

	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetTileState(EWordTileState NewState);

	UFUNCTION(BlueprintCallable, Category="Tile")
	void SetLocked(bool bLocked);

	/** True while the tile sits in the answer row rather than the bank. */
	bool bInAnswer = false;

protected:

	UPROPERTY(BlueprintReadOnly, Category="Tile", meta=(BindWidgetOptional))
	TObjectPtr<UButton> TileButton;

	UPROPERTY(BlueprintReadOnly, Category="Tile", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TileText;

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void HandleClicked();

	FString Word;
};
