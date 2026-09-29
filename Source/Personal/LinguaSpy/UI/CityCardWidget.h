// LinguaSpy - one city on the city-select screen

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CityCardWidget.generated.h"

class UButton;
class UTextBlock;
class ULanguagePackAsset;
class UCityCardWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityCardClicked, ULanguagePackAsset*, Pack);

/** Clickable card: city, language, tagline, mission progress. Builds a default layout in C++. */
UCLASS()
class UCityCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="City")
	FOnCityCardClicked OnCardClicked;

	void SetPack(ULanguagePackAsset* InPack, int32 CompletedMissions, int32 TotalMissions);

protected:

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CityText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TaglineText;

	UPROPERTY(BlueprintReadOnly, Category="City", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<ULanguagePackAsset> Pack;
};
