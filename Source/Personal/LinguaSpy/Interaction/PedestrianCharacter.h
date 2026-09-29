// LinguaSpy - ambient locals who wander the streets

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PedestrianCharacter.generated.h"

class USkeletalMesh;
class UMaterialInterface;

/**
 *  A non-interactive local that strolls between random reachable points around its spawn location,
 *  pausing now and then. Pick a random look from Variants on BeginPlay.
 */
UCLASS()
class APedestrianCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	APedestrianCharacter();

protected:

	/** How far from home the pedestrian wanders. */
	UPROPERTY(EditAnywhere, Category="Pedestrian", meta=(ClampMin="100"))
	float WanderRadius = 1200.f;

	/** Random pause between walks (seconds). */
	UPROPERTY(EditAnywhere, Category="Pedestrian")
	FVector2D PauseRange = FVector2D(1.5f, 6.f);

	/** Optional meshes to pick from at random (all on the mannequin skeleton). */
	UPROPERTY(EditAnywhere, Category="Pedestrian")
	TArray<TObjectPtr<USkeletalMesh>> MeshVariants;

	/** Optional materials to pick from at random for slot 0. */
	UPROPERTY(EditAnywhere, Category="Pedestrian")
	TArray<TObjectPtr<UMaterialInterface>> MaterialVariants;

	virtual void BeginPlay() override;

private:

	void ScheduleNextWalk();
	void Walk();

	FVector HomeLocation = FVector::ZeroVector;
	FTimerHandle WalkTimer;
};
