// LinguaSpy - per-city look presets: time of day, colour grading and toon post-process settings as data

#include "CityLook.h"
#include "Personal.h"
#include "EngineUtils.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FVector4 LerpV4(const FVector4& A, const FVector4& B, float T)
	{
		return A + (B - A) * T;
	}

	template <typename ValueType, typename LerpFn>
	TMap<FName, ValueType> BlendMaps(const TMap<FName, ValueType>& A, const TMap<FName, ValueType>& B, float T, LerpFn Lerp)
	{
		TMap<FName, ValueType> Out = A;
		for (const TPair<FName, ValueType>& Pair : B)
		{
			if (const ValueType* FromA = A.Find(Pair.Key))
			{
				Out.Add(Pair.Key, Lerp(*FromA, Pair.Value, T));
			}
			else
			{
				Out.Add(Pair.Key, Pair.Value);
			}
		}
		return Out;
	}
}

FCityLook UCityLookLibrary::BlendLooks(const FCityLook& A, const FCityLook& B, float Alpha)
{
	const float T = FMath::Clamp(Alpha, 0.f, 1.f);
	FCityLook Out;
	Out.SunRotation = FQuat::Slerp(A.SunRotation.Quaternion(), B.SunRotation.Quaternion(), T).Rotator();
	Out.SunColor = FMath::Lerp(A.SunColor, B.SunColor, T);
	Out.SunIntensity = FMath::Lerp(A.SunIntensity, B.SunIntensity, T);
	Out.SkyColor = FMath::Lerp(A.SkyColor, B.SkyColor, T);
	Out.SkyIntensity = FMath::Lerp(A.SkyIntensity, B.SkyIntensity, T);
	Out.FogColor = FMath::Lerp(A.FogColor, B.FogColor, T);
	Out.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, T);
	Out.FogStartDistance = FMath::Lerp(A.FogStartDistance, B.FogStartDistance, T);
	Out.WhiteTemp = FMath::Lerp(A.WhiteTemp, B.WhiteTemp, T);
	Out.Saturation = LerpV4(A.Saturation, B.Saturation, T);
	Out.Contrast = LerpV4(A.Contrast, B.Contrast, T);
	Out.GainShadows = LerpV4(A.GainShadows, B.GainShadows, T);
	Out.GainHighlights = LerpV4(A.GainHighlights, B.GainHighlights, T);
	Out.BloomIntensity = FMath::Lerp(A.BloomIntensity, B.BloomIntensity, T);
	Out.VignetteIntensity = FMath::Lerp(A.VignetteIntensity, B.VignetteIntensity, T);
	Out.AmbientOcclusionIntensity = FMath::Lerp(A.AmbientOcclusionIntensity, B.AmbientOcclusionIntensity, T);
	Out.DofFstop = FMath::Lerp(A.DofFstop, B.DofFstop, T);
	Out.DofSensorWidth = FMath::Lerp(A.DofSensorWidth, B.DofSensorWidth, T);
	Out.ToonScalars = BlendMaps(A.ToonScalars, B.ToonScalars, T, [](float X, float Y, float U) { return FMath::Lerp(X, Y, U); });
	Out.ToonColors = BlendMaps(A.ToonColors, B.ToonColors, T,
		[](const FLinearColor& X, const FLinearColor& Y, float U) { return FMath::Lerp(X, Y, U); });
	return Out;
}

// ---- Subsystem ---------------------------------------------------------------------------------------------------

TStatId UCityLookSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCityLookSubsystem, STATGROUP_Tickables);
}

APostProcessVolume* UCityLookSubsystem::FindToonVolume() const
{
	// The greybox kit puts one unbound volume ("ToonPostProcess") with PP_Toon as its blendable in every city map.
	// Actor labels don't exist in packaged builds, so find it by its blendable instead.
	APostProcessVolume* Fallback = nullptr;
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		if (!It->bUnbound)
		{
			continue;
		}
		Fallback = Fallback ? Fallback : *It;
		for (const FWeightedBlendable& Blendable : It->Settings.WeightedBlendables.Array)
		{
			const UMaterialInterface* Material = Cast<UMaterialInterface>(Blendable.Object);
			if (Material && Material->GetMaterial() && Material->GetMaterial()->GetName() == TEXT("PP_Toon"))
			{
				return *It;
			}
		}
	}
	return Fallback;
}

UMaterialInstanceDynamic* UCityLookSubsystem::GetToonMaterial(APostProcessVolume* Volume)
{
	if (ToonMID || !Volume)
	{
		return ToonMID;
	}
	for (FWeightedBlendable& Blendable : Volume->Settings.WeightedBlendables.Array)
	{
		UMaterialInterface* Material = Cast<UMaterialInterface>(Blendable.Object);
		if (Material && Material->GetMaterial() && Material->GetMaterial()->GetName() == TEXT("PP_Toon"))
		{
			ToonMID = UMaterialInstanceDynamic::Create(Material, this);
			Blendable.Object = ToonMID;
			break;
		}
	}
	return ToonMID;
}

void UCityLookSubsystem::ApplyLook(const FCityLook& Look)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	Current = Look;

	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (UDirectionalLightComponent* Sun = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			if (Sun->Mobility != EComponentMobility::Movable)
			{
				Sun->SetMobility(EComponentMobility::Movable);
			}
			Sun->SetWorldRotation(Look.SunRotation);
			Sun->SetLightColor(Look.SunColor);
			Sun->SetIntensity(Look.SunIntensity);
		}
		break; // the sun
	}
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		if (USkyLightComponent* Sky = It->GetLightComponent())
		{
			Sky->SetLightColor(Look.SkyColor);
			Sky->SetIntensity(Look.SkyIntensity);
		}
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* Fog = It->GetComponent())
		{
			Fog->SetFogInscatteringColor(Look.FogColor);
			Fog->SetFogDensity(Look.FogDensity);
			Fog->SetStartDistance(Look.FogStartDistance);
		}
	}

	APostProcessVolume* Volume = FindToonVolume();
	if (!Volume)
	{
		UE_LOG(LogPersonal, Warning, TEXT("CityLook: no unbound post-process volume in %s"), *World->GetName());
		return;
	}
	FPostProcessSettings& S = Volume->Settings;
	S.bOverride_WhiteTemp = true;               S.WhiteTemp = Look.WhiteTemp;
	S.bOverride_ColorSaturation = true;         S.ColorSaturation = Look.Saturation;
	S.bOverride_ColorContrast = true;           S.ColorContrast = Look.Contrast;
	S.bOverride_ColorGainShadows = true;        S.ColorGainShadows = Look.GainShadows;
	S.bOverride_ColorGainHighlights = true;     S.ColorGainHighlights = Look.GainHighlights;
	S.bOverride_BloomIntensity = true;          S.BloomIntensity = Look.BloomIntensity;
	S.bOverride_VignetteIntensity = true;       S.VignetteIntensity = Look.VignetteIntensity;
	S.bOverride_AmbientOcclusionIntensity = true; S.AmbientOcclusionIntensity = Look.AmbientOcclusionIntensity;
	S.bOverride_DepthOfFieldFstop = true;       S.DepthOfFieldFstop = Look.DofFstop;
	S.bOverride_DepthOfFieldSensorWidth = true; S.DepthOfFieldSensorWidth = Look.DofSensorWidth;

	if (UMaterialInstanceDynamic* Toon = GetToonMaterial(Volume))
	{
		for (const TPair<FName, float>& Pair : Look.ToonScalars)
		{
			Toon->SetScalarParameterValue(Pair.Key, Pair.Value);
		}
		for (const TPair<FName, FLinearColor>& Pair : Look.ToonColors)
		{
			Toon->SetVectorParameterValue(Pair.Key, Pair.Value);
		}
	}
}

void UCityLookSubsystem::BlendTo(const FCityLook& Target, float Seconds)
{
	if (Seconds <= 0.f)
	{
		BlendDuration = 0.f;
		ApplyLook(Target);
		return;
	}
	BlendFrom = Current;
	BlendTarget = Target;
	BlendElapsed = 0.f;
	BlendDuration = Seconds;
}

void UCityLookSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (BlendDuration <= 0.f)
	{
		return;
	}
	BlendElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(BlendElapsed / BlendDuration, 0.f, 1.f);
	ApplyLook(UCityLookLibrary::BlendLooks(BlendFrom, BlendTarget, FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f)));
	if (Alpha >= 1.f)
	{
		BlendDuration = 0.f;
	}
}
