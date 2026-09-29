// LinguaSpy - a local the spy can talk to

#include "NpcCharacter.h"
#include "ConversationSubsystem.h"
#include "MissionSubsystem.h"
#include "MissionTargetComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ANpcCharacter::ANpcCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	// Block visibility traces so the cursor can click on NPCs
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Default body: the mannequin with the unarmed idle (replaced by the stylized characters in the art pass)
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

void ANpcCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateBodyVisibility();
}

void ANpcCharacter::UpdateBodyVisibility()
{
	const bool bHasSkeletal = GetMesh() && GetMesh()->GetSkeletalMeshAsset() != nullptr;
	BodyMesh->SetVisibility(!bHasSkeletal);
}

void ANpcCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateBodyVisibility();
	HomeYaw = GetActorRotation().Yaw;
	TargetYaw = HomeYaw;

	if (UConversationSubsystem* Conversations = GetWorld()->GetSubsystem<UConversationSubsystem>())
	{
		Conversations->OnConversationStarted.AddUniqueDynamic(this, &ANpcCharacter::HandleConversationStarted);
		Conversations->OnConversationEnded.AddUniqueDynamic(this, &ANpcCharacter::HandleConversationEnded);
	}
}

void ANpcCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UConversationSubsystem* Conversations = World->GetSubsystem<UConversationSubsystem>())
		{
			Conversations->OnConversationStarted.RemoveAll(this);
			Conversations->OnConversationEnded.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ANpcCharacter::HandleConversationStarted(ANpcCharacter* Npc)
{
	if (Npc != this)
	{
		return;
	}
	bTalking = true;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		TargetYaw = (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		SetActorTickEnabled(true);
	}
}

void ANpcCharacter::HandleConversationEnded()
{
	if (bTalking)
	{
		bTalking = false;
		TargetYaw = HomeYaw;
		SetActorTickEnabled(true);
	}
}

void ANpcCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float CurrentYaw = GetActorRotation().Yaw;
	const float Delta = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	const float Step = TurnSpeed * DeltaSeconds;
	if (FMath::Abs(Delta) <= Step)
	{
		SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
		SetActorTickEnabled(false);
	}
	else
	{
		SetActorRotation(FRotator(0.f, CurrentYaw + FMath::Sign(Delta) * Step, 0.f));
	}
}

int32 ANpcCharacter::ChooseConversationOption(const TArray<FNpcConversationOption>& Options, FName ActiveMissionId,
	bool bIsCurrentTarget, TFunctionRef<bool(FName)> HasFlag)
{
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		const FNpcConversationOption& Option = Options[i];
		if (!Option.Conversation)
		{
			continue;
		}
		if (!Option.RequiredMission.IsNone() && Option.RequiredMission != ActiveMissionId)
		{
			continue;
		}
		if (Option.bOnlyIfCurrentTarget && !bIsCurrentTarget)
		{
			continue;
		}
		const bool bMissingRequired = Option.RequiredFlags.ContainsByPredicate([&HasFlag](FName Flag) { return !HasFlag(Flag); });
		const bool bHasBlocked = Option.BlockedFlags.ContainsByPredicate([&HasFlag](FName Flag) { return HasFlag(Flag); });
		if (!bMissingRequired && !bHasBlocked)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

UConversationAsset* ANpcCharacter::ResolveConversation() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UMissionSubsystem* Missions = GameInstance ? GameInstance->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (Missions)
	{
		const UMissionAsset* Mission = Missions->GetActiveMission();
		const FName MissionId = Mission ? Mission->MissionId : NAME_None;
		const bool bIsTarget = !GetMissionTargetId().IsNone() && Missions->GetCurrentTargetId() == GetMissionTargetId();
		const int32 Index = ChooseConversationOption(ConversationOptions, MissionId, bIsTarget,
			[Missions](FName Flag) { return Missions->HasFlag(Flag); });
		if (Index != INDEX_NONE)
		{
			return ConversationOptions[Index].Conversation;
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
