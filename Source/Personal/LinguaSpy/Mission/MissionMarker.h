// LinguaSpy - a place the spy must reach

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionMarker.generated.h"

class UBoxComponent;
class UMissionTargetComponent;
class UStaticMeshComponent;

/** Trigger volume for ReachLocation objectives. Shows a ground ring + beacon while it is the current target. */
UCLASS()
class AMissionMarker : public AActor
{
	GENERATED_BODY()

public:

	AMissionMarker();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> GroundRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UMissionTargetComponent> MissionTarget;

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTargetActiveChanged(bool bIsCurrentTarget);
};
