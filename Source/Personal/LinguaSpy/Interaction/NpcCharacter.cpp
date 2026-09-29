// LinguaSpy - a local the spy can talk to

#include "NpcCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ANpcCharacter::ANpcCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	// Block visibility traces so the cursor can click on NPCs
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CapsuleMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CapsuleMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.9f));
	}

	TalkSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TalkSphere"));
	TalkSphere->SetupAttachment(GetCapsuleComponent());
	TalkSphere->InitSphereRadius(200.f);
	TalkSphere->SetCollisionProfileName(TEXT("NoCollision"));

	DisplayName = NSLOCTEXT("LinguaSpy", "DefaultNpcName", "Local");
}

float ANpcCharacter::GetTalkRadius() const
{
	return TalkSphere->GetScaledSphereRadius();
}
