// LinguaSpy - per-city look presets: time of day, colour grading and toon post-process settings as data

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "CityLook.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;

/**
 *  Everything that gives a city its mood: the sun (time of day), sky and fog colour, colour grading on the
 *  ToonPostProcess volume, tilt-shift depth of field, and PP_Toon parameters (band thresholds, haze, outline...).
 *  Plain data so it can be authored per city (UCityLookAsset), blended (UCityLookLibrary::BlendLooks) and tested.
 */
USTRUCT(BlueprintType)
struct PERSONAL_API FCityLook
{
	GENERATED_BODY()

	// ---- Sun / sky / fog
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	FRotator SunRotation = FRotator(-50.f, 30.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	FLinearColor SunColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="0"))
	float SunIntensity = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	FLinearColor SkyColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="0"))
	float SkyIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
	FLinearColor FogColor = FLinearColor(0.6f, 0.7f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta=(ClampMin="0"))
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta=(ClampMin="0"))
	float FogStartDistance = 1500.f;

	// ---- Grading (ToonPostProcess volume)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading")
	float WhiteTemp = 6500.f;

	/** RGB = per-channel, W = overall (like the post-process colour wheels). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading")
	FVector4 Saturation = FVector4(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading")
	FVector4 Contrast = FVector4(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading")
	FVector4 GainShadows = FVector4(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading")
	FVector4 GainHighlights = FVector4(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading", meta=(ClampMin="0"))
	float BloomIntensity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading", meta=(ClampMin="0"))
	float VignetteIntensity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grading", meta=(ClampMin="0", ClampMax="1"))
	float AmbientOcclusionIntensity = 0.5f;

	// ---- Tilt-shift depth of field (focus distance follows the camera boom, see APersonalCharacter)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Depth of field", meta=(ClampMin="0.1"))
	float DofFstop = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Depth of field", meta=(ClampMin="1"))
	float DofSensorWidth = 36.f;

	// ---- PP_Toon material parameters (applied through a dynamic instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toon")
	TMap<FName, float> ToonScalars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toon")
	TMap<FName, FLinearColor> ToonColors;
};

/** A city's look, referenced from ULanguagePackAsset::Look. */
UCLASS(BlueprintType)
class PERSONAL_API UCityLookAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Look", meta=(ShowOnlyInnerProperties))
	FCityLook Look;
};

/** Pure look math (no world access), so it can be unit tested. */
UCLASS()
class PERSONAL_API UCityLookLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  Linear blend from A (Alpha 0) to B (Alpha 1); Alpha is clamped to [0, 1].
	 *  Colours blend in linear space, the sun rotation along the shortest arc (quaternion slerp),
	 *  and toon parameters per key: a key only one side has keeps that side's value.
	 */
	UFUNCTION(BlueprintPure, Category="LinguaSpy|Look")
	static FCityLook BlendLooks(const FCityLook& A, const FCityLook& B, float Alpha);
};

/**
 *  Applies a FCityLook to the running world: the first directional light, sky light, height fog and the unbound
 *  ToonPostProcess volume (its PP_Toon blendable is swapped for a dynamic instance). BlendTo eases between looks.
 */
UCLASS()
class PERSONAL_API UCityLookSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Applies the look immediately. */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Look")
	void ApplyLook(const FCityLook& Look);

	/** Eases from the current look to Target over Seconds (0 = immediately). */
	UFUNCTION(BlueprintCallable, Category="LinguaSpy|Look")
	void BlendTo(const FCityLook& Target, float Seconds);

	UFUNCTION(BlueprintPure, Category="LinguaSpy|Look")
	FCityLook GetCurrentLook() const { return Current; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return BlendDuration > 0.f; }

private:

	APostProcessVolume* FindToonVolume() const;
	UMaterialInstanceDynamic* GetToonMaterial(APostProcessVolume* Volume);

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ToonMID;

	FCityLook Current;
	FCityLook BlendFrom;
	FCityLook BlendTarget;
	float BlendElapsed = 0.f;
	float BlendDuration = 0.f;
};
