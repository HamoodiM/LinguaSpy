// LinguaSpy - always-on HUD: cover meter, objective tracker, mission banners

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpyHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UBorder;
class UCoverComponent;
class UMissionAsset;
class UMissionSubsystem;

/** Shows the spy's cover meter, current objective and mission banners. Builds a default layout in C++. */
UCLASS()
class USpyHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Binds the cover meter to the given component. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void BindCover(UCoverComponent* Cover);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetHint(const FText& Hint);

	/** Shows a large centred message for a few seconds (queued behind a banner that is still showing). */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowBanner(const FText& Title, const FText& Subtitle, float Duration = 4.f);

	/** A banner is showing, held, or queued (the controller waits for this before opening the next briefing). */
	bool IsBannerBusy() const { return BannerTimeLeft > 0.f || BannerQueue.Num() > 0; }

protected:

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> CoverBar;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CoverLabel;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MissionTitleText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UBorder> BannerPanel;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerTitle;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerSubtitle;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildDefaultLayout();
	UMissionSubsystem* GetMissionSubsystem() const;

	UFUNCTION()
	void HandleCoverChanged(float NewCover, float MaxCover);

	UFUNCTION()
	void HandleObjectiveChanged();

	UFUNCTION()
	void HandleMissionStarted(UMissionAsset* Mission);

	UFUNCTION()
	void HandleMissionCompleted(UMissionAsset* Mission);

	UFUNCTION()
	void HandleMissionFailed(UMissionAsset* Mission);

	UFUNCTION()
	void HandleCityCompleted();

	void DisplayBanner(const FText& Title, const FText& Subtitle, float Duration);

	/**
	 *  Banners never share the screen with a conversation/debrief panel or a pausing overlay (briefing, field note,
	 *  pause menu): while one is up, the current banner is hidden with its time frozen and new ones queue.
	 */
	bool IsBannerHeld() const;
	bool bBannerSuspended = false;

	struct FPendingBanner
	{
		FText Title;
		FText Subtitle;
		float Duration = 4.f;
	};
	TArray<FPendingBanner> BannerQueue;

	TWeakObjectPtr<UCoverComponent> BoundCover;
	float BannerTimeLeft = 0.f;
	float BannerPulseTime = -1.f;
};
