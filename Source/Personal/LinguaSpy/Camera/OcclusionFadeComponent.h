// LinguaSpy - fades out scenery between the top-down camera and the spy

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OcclusionFadeComponent.generated.h"

class UPrimitiveComponent;

/**
 *  Every tick, sweeps from the view to the owning pawn and fades occluding primitives by writing
 *  Custom Primitive Data [FadeDataIndex] (0 = solid, 1 = faded). The toon master material turns that value into
 *  a dithered opacity mask, so no dynamic material instances are needed.
 */
UCLASS(ClassGroup=(LinguaSpy), meta=(BlueprintSpawnableComponent))
class PERSONAL_API UOcclusionFadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOcclusionFadeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Sweep radius around the camera-to-pawn line (wider since the camera moved back to 25 m). */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	float SweepRadius = 110.f;

	/** How fast primitives fade in/out (fade units per second). */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	float FadeSpeed = 4.f;

	/** Maximum fade (1 = fully dithered away). */
	UPROPERTY(EditAnywhere, Category="Occlusion", meta=(ClampMin="0", ClampMax="1"))
	float MaxFade = 0.85f;

	/** Only fade things whose top is at least this far above the pawn's feet (keeps the ground solid). */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	float MinHeightAboveFeet = 120.f;

	/** Custom Primitive Data slot read by the material. */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	int32 FadeDataIndex = 0;

private:

	void SetFade(UPrimitiveComponent* Primitive, float Value) const;

	/** Current fade per primitive. Entries are removed once fully visible again. */
	TMap<TWeakObjectPtr<UPrimitiveComponent>, float> Faded;
};
