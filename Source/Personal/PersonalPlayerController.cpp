// Copyright Epic Games, Inc. All Rights Reserved.

#include "PersonalPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "PersonalCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Personal.h"
#include "NpcCharacter.h"
#include "CoverComponent.h"
#include "ConversationSubsystem.h"
#include "ConversationWidget.h"
#include "SpyHUDWidget.h"
#include "Blueprint/UserWidget.h"

APersonalPlayerController::APersonalPlayerController()
{
	bIsTouch = false;
	bMoveToMouseCursor = false;

	// create the path following comp
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("Path Following Component"));

	// configure the controller
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;

	ConversationWidgetClass = UConversationWidget::StaticClass();
	HUDWidgetClass = USpyHUDWidget::StaticClass();
}

void APersonalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<USpyHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport(0);
		if (const APersonalCharacter* SpyCharacter = Cast<APersonalCharacter>(GetPawn()))
		{
			HUDWidget->BindCover(SpyCharacter->GetCover());
		}
	}
	if (ConversationWidgetClass)
	{
		ConversationWidget = CreateWidget<UConversationWidget>(this, ConversationWidgetClass);
		ConversationWidget->AddToViewport(10);
	}

	if (UConversationSubsystem* Conversations = GetWorld()->GetSubsystem<UConversationSubsystem>())
	{
		Conversations->OnConversationStarted.AddUniqueDynamic(this, &APersonalPlayerController::HandleConversationStarted);
		Conversations->OnConversationEnded.AddUniqueDynamic(this, &APersonalPlayerController::HandleConversationEnded);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void APersonalPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HUDWidget)
	{
		if (const APersonalCharacter* SpyCharacter = Cast<APersonalCharacter>(InPawn))
		{
			HUDWidget->BindCover(SpyCharacter->GetCover());
		}
	}
}

bool APersonalPlayerController::IsInConversation() const
{
	const UConversationSubsystem* Conversations = GetWorld() ? GetWorld()->GetSubsystem<UConversationSubsystem>() : nullptr;
	return Conversations && Conversations->IsActive();
}

void APersonalPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ANpcCharacter* Npc = PendingTalkTarget.Get();
	APawn* ControlledPawn = GetPawn();
	if (!Npc || !ControlledPawn || IsInConversation())
	{
		return;
	}

	const FVector ToNpc = Npc->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToNpc.Size2D() <= Npc->GetTalkRadius())
	{
		PendingTalkTarget.Reset();
		StopMovement();
		ControlledPawn->SetActorRotation(FRotator(0.f, ToNpc.Rotation().Yaw, 0.f));

		const APersonalCharacter* SpyCharacter = Cast<APersonalCharacter>(ControlledPawn);
		if (UConversationSubsystem* Conversations = GetWorld()->GetSubsystem<UConversationSubsystem>())
		{
			Conversations->StartConversation(Npc, SpyCharacter ? SpyCharacter->GetCover() : nullptr);
		}
	}
}

void APersonalPlayerController::HandleConversationStarted(ANpcCharacter* Npc)
{
	StopMovement();
	if (HUDWidget)
	{
		HUDWidget->SetHint(FText::Format(NSLOCTEXT("LinguaSpy", "TalkingHint", "Talking to {0}. Stay in character."), Npc ? Npc->GetDisplayName() : FText::GetEmpty()));
	}
}

void APersonalPlayerController::HandleConversationEnded()
{
	if (HUDWidget)
	{
		HUDWidget->SetHint(NSLOCTEXT("LinguaSpy", "DefaultHint", "Click a local to talk to them."));
	}
}

void APersonalPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Setup mouse input events
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &APersonalPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &APersonalPlayerController::OnSetDestinationTriggered);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &APersonalPlayerController::OnSetDestinationReleased);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &APersonalPlayerController::OnSetDestinationReleased);

			// Setup touch input events
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &APersonalPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &APersonalPlayerController::OnTouchTriggered);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &APersonalPlayerController::OnTouchReleased);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &APersonalPlayerController::OnTouchReleased);
		}
		else
		{
			UE_LOG(LogPersonal, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
		}
	}
}

void APersonalPlayerController::OnInputStarted()
{
	bClickTargetedNpc = false;
	if (IsInConversation())
	{
		return;
	}

	StopMovement();
	PendingTalkTarget.Reset();

	// Clicking a local walks over to talk to them
	FHitResult NpcHit;
	const bool bHit = bIsTouch
		? GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, NpcHit)
		: GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, NpcHit);
	if (ANpcCharacter* Npc = bHit ? Cast<ANpcCharacter>(NpcHit.GetActor()) : nullptr)
	{
		bClickTargetedNpc = true;
		PendingTalkTarget = Npc;
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Npc->GetActorLocation());
		return;
	}

	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();
}

void APersonalPlayerController::OnSetDestinationTriggered()
{
	if (bClickTargetedNpc || IsInConversation())
	{
		return;
	}

	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void APersonalPlayerController::OnSetDestinationReleased()
{
	if (bClickTargetedNpc || IsInConversation())
	{
		bClickTargetedNpc = false;
		FollowTime = 0.f;
		return;
	}

	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
void APersonalPlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void APersonalPlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void APersonalPlayerController::UpdateCachedDestination()
{
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
}
