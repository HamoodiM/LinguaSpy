// LinguaSpy - in-city pause menu

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UOverlayPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseResume);

/** Resume / phrasebook / settings / return to menu. Builds a default layout in C++. */
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

	/** Refreshes the mission tally line (called when the menu opens). */
	void RefreshLabels();

protected:

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PhrasebookButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> MenuButton;

	/** "This mission so far: 2 mistakes, 3 peeks". */
	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TallyText;

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:

	/** Phrasebook / settings opened from here; closed together with the pause menu. */
	TArray<TWeakObjectPtr<UOverlayPanelWidget>> OpenOverlays;

	void BuildDefaultLayout();

	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandlePhrasebook();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleMenu();
};
