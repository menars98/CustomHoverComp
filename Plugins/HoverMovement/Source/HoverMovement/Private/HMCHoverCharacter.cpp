// Fill out your copyright notice in the Description page of Project Settings.


#include "HMCHoverCharacter.h"
#include "HMCHoverMovementComponent.h"

AHMCHoverCharacter::AHMCHoverCharacter(const FObjectInitializer& ObjectInitializer)
	: ACharacter(ObjectInitializer.SetDefaultSubobjectClass<UHMCHoverMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;


}

UHMCHoverMovementComponent* AHMCHoverCharacter::GetHoverMovementComponent() const
{
	return Cast<UHMCHoverMovementComponent>(GetCharacterMovement());
}
