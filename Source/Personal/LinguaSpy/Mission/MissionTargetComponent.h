// LinguaSpy - makes an actor addressable by missions and shows a beacon when it is the current target

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "MissionTargetComponent.generated.h"

class UMissionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetActiveChanged, bool, bIsCurrentTarget);

/**
 *  Floating beacon mesh + mission id. Visible only while its TargetId is the current objective's target.
 *  Attach it above the owning actor (NPC, marker, pickup).
 */
UCLASS(ClassGroup=(LinguaSpy), meta=(BlueprintSpawnableComponent))
class UMissionTargetComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UMissionTargetComponent();

	/** Id that mission objectives use to point at this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission")
	FName TargetId;

	UPROPERTY(BlueprintAssignable, Category="Mission")
	FOnTargetActiveChanged OnTargetActiveChanged;

	UFUNCTION(BlueprintPure, Category="Mission")
	bool IsCurrentTarget() const { return bIsCurrentTarget; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Bob height and speed for the beacon. */
	UPROPERTY(EditAnywhere, Category="Mission")
	float BobHeight = 15.f;

	UPROPERTY(EditAnywhere, Category="Mission")
	float SpinSpeed = 90.f;

private:

	UFUNCTION()
	void RefreshTarget();

	UMissionSubsystem* GetMissionSubsystem() const;

	bool bIsCurrentTarget = false;
	FVector BaseLocation = FVector::ZeroVector;
	float Age = 0.f;
};
