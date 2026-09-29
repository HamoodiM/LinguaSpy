// LinguaSpy - a local the spy can talk to

#include "NpcCharacter.h"
#include "MissionSubsystem.h"
#include "MissionTargetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
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

	MissionTarget = CreateDefaultSubobject<UMissionTargetComponent>(TEXT("MissionTarget"));
	MissionTarget->SetupAttachment(GetCapsuleComponent());
	MissionTarget->SetRelativeLocation(FVector(0.f, 0.f, 160.f));

	DisplayName = NSLOCTEXT("LinguaSpy", "DefaultNpcName", "Local");
}

UConversationAsset* ANpcCharacter::ResolveConversation() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UMissionSubsystem* Missions = GameInstance ? GameInstance->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Missions)
	{
		for (const FNpcConversationOption& Option : ConversationOptions)
		{
			if (Option.Conversation && Missions->HasAllFlags(Option.RequiredFlags) && !Missions->HasAnyFlag(Option.BlockedFlags))
			{
				return Option.Conversation;
			}
		}
	}
	return Conversation;
}

FName ANpcCharacter::GetMissionTargetId() const
{
	return MissionTarget->TargetId;
}

float ANpcCharacter::GetTalkRadius() const
{
	return TalkSphere->GetScaledSphereRadius();
}
