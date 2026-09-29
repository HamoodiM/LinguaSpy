// LinguaSpy - ambient locals who wander the streets

#include "PedestrianCharacter.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APedestrianCharacter::APedestrianCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(38.f, 96.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mannequin(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (Mannequin.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(Mannequin.Object);
	}
	if (UnarmedAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(UnarmedAnim.Class);
	}
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -95.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// Don't block the cursor: clicking a pedestrian should move the spy, not select them
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 360.f, 0.f);
	Movement->MaxWalkSpeed = 160.f;
	Movement->GetNavMovementProperties()->bUseAccelerationForPaths = true; // drives the walk animation
	bUseControllerRotationYaw = false;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void APedestrianCharacter::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();

	if (MeshVariants.Num() > 0)
	{
		if (USkeletalMesh* Mesh = MeshVariants[FMath::RandRange(0, MeshVariants.Num() - 1)])
		{
			GetMesh()->SetSkeletalMesh(Mesh);
		}
	}
	if (MaterialVariants.Num() > 0)
	{
		if (UMaterialInterface* Material = MaterialVariants[FMath::RandRange(0, MaterialVariants.Num() - 1)])
		{
			GetMesh()->SetMaterial(0, Material);
		}
	}
	GetCharacterMovement()->MaxWalkSpeed = FMath::RandRange(120.f, 190.f);

	ScheduleNextWalk();
}

void APedestrianCharacter::ScheduleNextWalk()
{
	GetWorldTimerManager().SetTimer(WalkTimer, this, &APedestrianCharacter::Walk, FMath::RandRange(PauseRange.X, PauseRange.Y), false);
}

void APedestrianCharacter::Walk()
{
	AAIController* AI = Cast<AAIController>(GetController());
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (AI && Nav)
	{
		// Only pick a new destination once the previous walk is done
		if (AI->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			FNavLocation Destination;
			if (Nav->GetRandomReachablePointInRadius(HomeLocation, WanderRadius, Destination))
			{
				AI->MoveToLocation(Destination.Location, 30.f);
			}
		}
	}
	// Check again after a pause (also covers "still walking")
	ScheduleNextWalk();
}
