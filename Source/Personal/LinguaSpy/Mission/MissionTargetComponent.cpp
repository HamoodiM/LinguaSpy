// LinguaSpy - makes an actor addressable by missions and shows a beacon when it is the current target

#include "MissionTargetComponent.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UMissionTargetComponent::UMissionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		SetStaticMesh(ConeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeaconMaterial(TEXT("/Game/LinguaSpy/Materials/MI_Butter.MI_Butter"));
	if (BeaconMaterial.Succeeded())
	{
		SetMaterial(0, BeaconMaterial.Object);
	}

	// Upside-down cone floating above the owner
	SetRelativeLocation(FVector(0.f, 0.f, 170.f));
	SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
	SetRelativeScale3D(FVector(0.35f, 0.35f, 0.45f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetHiddenInGame(true);
	SetCanEverAffectNavigation(false);
}

UMissionSubsystem* UMissionTargetComponent::GetMissionSubsystem() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMissionSubsystem>() : nullptr;
}

void UMissionTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseLocation = GetRelativeLocation();
	if (UMissionSubsystem* Missions = GetMissionSubsystem())
	{
		Missions->OnObjectiveChanged.AddUniqueDynamic(this, &UMissionTargetComponent::RefreshTarget);
	}
	RefreshTarget();
}

void UMissionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionSubsystem* Missions = GetMissionSubsystem())
	{
		Missions->OnObjectiveChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UMissionTargetComponent::RefreshTarget()
{
	const UMissionSubsystem* Missions = GetMissionSubsystem();
	const bool bNowTarget = Missions && !TargetId.IsNone() && Missions->GetCurrentTargetId() == TargetId;
	SetHiddenInGame(!bNowTarget);
	SetComponentTickEnabled(bNowTarget);
	if (bNowTarget != bIsCurrentTarget)
	{
		bIsCurrentTarget = bNowTarget;
		OnTargetActiveChanged.Broadcast(bIsCurrentTarget);
	}
}

void UMissionTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Age += DeltaTime;
	SetRelativeLocation(BaseLocation + FVector(0.f, 0.f, FMath::Sin(Age * 3.f) * BobHeight));
	SetRelativeRotation(FRotator(180.f, Age * SpinSpeed, 0.f));
}
