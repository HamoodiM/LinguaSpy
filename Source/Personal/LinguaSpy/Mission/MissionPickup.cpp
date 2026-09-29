// LinguaSpy - an item the spy must collect

#include "MissionPickup.h"
#include "MissionTargetComponent.h"
#include "MissionSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AMissionPickup::AMissionPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(90.f);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetCanEverAffectNavigation(false);
	RootComponent = Trigger;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(Trigger);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		ItemMesh->SetStaticMesh(Cube.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ItemMaterial(TEXT("/Game/LinguaSpy/Materials/MI_Wood.MI_Wood"));
	if (ItemMaterial.Succeeded())
	{
		ItemMesh->SetMaterial(0, ItemMaterial.Object);
	}
	ItemMesh->SetRelativeScale3D(FVector(0.4f, 0.3f, 0.25f));
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetCanEverAffectNavigation(false);

	MissionTarget = CreateDefaultSubobject<UMissionTargetComponent>(TEXT("MissionTarget"));
	MissionTarget->SetupAttachment(Trigger);
	MissionTarget->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
}

void AMissionPickup::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMissionPickup::HandleOverlap);
	MissionTarget->OnTargetActiveChanged.AddDynamic(this, &AMissionPickup::HandleTargetActiveChanged);
	HandleTargetActiveChanged(MissionTarget->IsCurrentTarget());
}

void AMissionPickup::HandleTargetActiveChanged(bool bIsCurrentTarget)
{
	// The item only exists in the world while the mission wants it
	ItemMesh->SetHiddenInGame(!bIsCurrentTarget);
	Trigger->SetGenerateOverlapEvents(bIsCurrentTarget);
}

void AMissionPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled() || !MissionTarget->IsCurrentTarget())
	{
		return;
	}
	if (UMissionSubsystem* Missions = GetGameInstance()->GetSubsystem<UMissionSubsystem>())
	{
		Missions->ReportCollected(MissionTarget->TargetId);
	}
}
