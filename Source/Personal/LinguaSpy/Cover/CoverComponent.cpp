// LinguaSpy - the spy's cover meter

#include "CoverComponent.h"

UCoverComponent::UCoverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCoverComponent::BeginPlay()
{
	Super::BeginPlay();
	SetCover(MaxCover);
}

void UCoverComponent::ApplyPenalty(float Amount)
{
	if (IsBlown())
	{
		return;
	}
	SetCover(Cover - FMath::Max(0.f, Amount));
	if (IsBlown())
	{
		OnCoverBlown.Broadcast();
	}
}

void UCoverComponent::Recover(float Amount)
{
	SetCover(Cover + FMath::Max(0.f, Amount));
}

void UCoverComponent::ResetCover()
{
	SetCover(MaxCover);
}

void UCoverComponent::SetCover(float NewValue)
{
	Cover = FMath::Clamp(NewValue, 0.f, MaxCover);
	OnCoverChanged.Broadcast(Cover, MaxCover);
}
