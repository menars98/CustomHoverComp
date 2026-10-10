// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HMCHoverCharacter.generated.h"

class UHMCHoverMovementComponent;

UCLASS()
class HOVERMOVEMENT_API AHMCHoverCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// ObjectInitializer constructor
	AHMCHoverCharacter(const FObjectInitializer& ObjectInitializer);

	/** Getter */
	UFUNCTION(BlueprintPure, Category = "HMC|Movement")
	UHMCHoverMovementComponent* GetHoverMovementComponent() const;

};
