// LinguaSpy - an item the spy must collect

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMissionTargetComponent;

/** Item for Collect objectives. Only visible while it is the current target; walking into it collects it. */
UCLASS()
class AMissionPickup : public AActor
{
	GENERATED_BODY()

public:

	AMissionPickup();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> Trigger;

	/** Stylized placeholder item (a small parcel by default). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

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
