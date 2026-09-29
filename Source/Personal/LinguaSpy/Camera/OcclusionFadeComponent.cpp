// LinguaSpy - fades out scenery between the top-down camera and the spy

#include "OcclusionFadeComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UOcclusionFadeComponent::UOcclusionFadeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UOcclusionFadeComponent::SetFade(UPrimitiveComponent* Primitive, float Value) const
{
	Primitive->SetCustomPrimitiveDataFloat(FadeDataIndex, Value);
}

void UOcclusionFadeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const FVector ViewLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector Target = Pawn->GetActorLocation();
	const float FeetZ = Target.Z - Pawn->GetSimpleCollisionHalfHeight();

	// Object-type sweeps report every hit along the way (nothing "blocks" an object query)
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LinguaSpyOcclusion), false, Pawn);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, ViewLocation, Target, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(SweepRadius), Params);

	TSet<UPrimitiveComponent*> Occluders;
	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* Primitive = Hit.GetComponent();
		if (!Primitive || Cast<ACharacter>(Primitive->GetOwner()))
		{
			continue;
		}
		const FBoxSphereBounds& Bounds = Primitive->Bounds;
		if (Bounds.Origin.Z + Bounds.BoxExtent.Z < FeetZ + MinHeightAboveFeet)
		{
			continue; // ground, pavement, low props
		}
		Occluders.Add(Primitive);
		Faded.FindOrAdd(Primitive, 0.f);
	}

	for (auto It = Faded.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Primitive = It->Key.Get();
		if (!Primitive)
		{
			It.RemoveCurrent();
			continue;
		}
		const float TargetFade = Occluders.Contains(Primitive) ? MaxFade : 0.f;
		It->Value = FMath::FInterpConstantTo(It->Value, TargetFade, DeltaTime, FadeSpeed);
		SetFade(Primitive, It->Value);
		if (It->Value <= 0.f && TargetFade <= 0.f)
		{
			It.RemoveCurrent();
		}
	}
}

void UOcclusionFadeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, float>& Pair : Faded)
	{
		if (UPrimitiveComponent* Primitive = Pair.Key.Get())
		{
			SetFade(Primitive, 0.f);
		}
	}
	Faded.Reset();
	Super::EndPlay(EndPlayReason);
}
