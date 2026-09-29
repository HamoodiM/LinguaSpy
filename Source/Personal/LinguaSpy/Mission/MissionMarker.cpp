// LinguaSpy - a place the spy must reach

#include "MissionMarker.h"
#include "MissionTargetComponent.h"
#include "MissionSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AMissionMarker::AMissionMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetCanEverAffectNavigation(false);
	RootComponent = Trigger;

	GroundRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundRing"));
	GroundRing->SetupAttachment(Trigger);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		GroundRing->SetStaticMesh(Cylinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RingMaterial(TEXT("/Game/LinguaSpy/Materials/MI_Butter.MI_Butter"));
	if (RingMaterial.Succeeded())
	{
		GroundRing->SetMaterial(0, RingMaterial.Object);
	}
	GroundRing->SetRelativeLocation(FVector(0.f, 0.f, -98.f));
	GroundRing->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.02f));
	GroundRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundRing->SetCastShadow(false);
	GroundRing->SetCanEverAffectNavigation(false);
	GroundRing->SetHiddenInGame(true);

	MissionTarget = CreateDefaultSubobject<UMissionTargetComponent>(TEXT("MissionTarget"));
	MissionTarget->SetupAttachment(Trigger);
}

void AMissionMarker::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMissionMarker::HandleOverlap);
	MissionTarget->OnTargetActiveChanged.AddDynamic(this, &AMissionMarker::HandleTargetActiveChanged);
	HandleTargetActiveChanged(MissionTarget->IsCurrentTarget());
}

void AMissionMarker::HandleTargetActiveChanged(bool bIsCurrentTarget)
{
	GroundRing->SetHiddenInGame(!bIsCurrentTarget);

	// If the player is already standing inside when this becomes the target, count it
	if (bIsCurrentTarget)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, APawn::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			const APawn* Pawn = Cast<APawn>(Actor);
			if (Pawn && Pawn->IsPlayerControlled())
			{
				if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
				{
					Missions->ReportMarkerReached(MissionTarget->TargetId);
				}
				break;
			}
		}
	}
}

void AMissionMarker::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->ReportMarkerReached(MissionTarget->TargetId);
	}
}
