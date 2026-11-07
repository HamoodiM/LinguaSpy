// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersonalCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "CoverComponent.h"
#include "OcclusionFadeComponent.h"

APersonalCharacter::APersonalCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Click-to-move path following must generate acceleration, otherwise the
	// anim BP (which gates ShouldMove on acceleration) stays in idle while sliding
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	// Diorama framing: look ACROSS the street (streets run along X, shopfronts face +-Y) with a long lens, so the
	// north facades face the camera instead of being seen edge-on. Was 800 cm / pitch -60 / yaw 0 / FOV 90.
	CameraBoom->TargetArmLength = 2500.f;
	CameraBoom->SetRelativeRotation(FRotator(-37.f, 90.f, 0.f));
	CameraBoom->TargetOffset = FVector(0.f, 100.f, 0.f); // aim slightly north of the spy: more facade, less road
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.f;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	TopDownCameraComponent->SetFieldOfView(40.f);
	// Tilt-shift: focus distance tracks the spy every tick (f-stop / sensor come from the city's look preset)
	TopDownCameraComponent->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	TopDownCameraComponent->PostProcessSettings.DepthOfFieldFocalDistance = 2500.f;
	TopDownCameraComponent->PostProcessBlendWeight = 1.f;

	// Create the cover meter
	Cover = CreateDefaultSubobject<UCoverComponent>(TEXT("Cover"));

	// Fade buildings/awnings between the camera and the spy
	OcclusionFade = CreateDefaultSubobject<UOcclusionFadeComponent>(TEXT("OcclusionFade"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void APersonalCharacter::BeginPlay()
{
	Super::BeginPlay();

	// stub
}

void APersonalCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	// Keep the spy in the depth-of-field focal plane (camera lag changes the distance a little while walking)
	if (TopDownCameraComponent)
	{
		TopDownCameraComponent->PostProcessSettings.DepthOfFieldFocalDistance =
			FVector::Dist(TopDownCameraComponent->GetComponentLocation(), GetActorLocation());
	}
}
