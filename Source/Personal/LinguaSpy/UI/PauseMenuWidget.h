// LinguaSpy - in-city pause menu

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseResume);

/** Resume / settings toggles / return to menu. Builds a default layout in C++. */
UCLASS()
class UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Pause")
	FOnPauseResume OnResume;

	/** Map opened by "Return to menu". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pause")
	TSoftObjectPtr<UWorld> MenuMap;

	void RefreshLabels();

protected:

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> HelperButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HelperLabel;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ReadingsButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadingsLabel;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> MenuButton;

	virtual void NativeOnInitialized() override;

private:

	void BuildDefaultLayout();

	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleHelper();

	UFUNCTION()
	void HandleReadings();

	UFUNCTION()
	void HandleMenu();
};
