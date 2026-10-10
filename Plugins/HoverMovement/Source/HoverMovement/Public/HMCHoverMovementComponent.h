// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HMCHoverMovementComponent.generated.h"

/**
 * Enum for custom movement modes. This is used to define the different movement modes that the hover movement component can use.
 */
UENUM(BlueprintType)
enum class EHMC_CustomMovementMode : uint8
{
	None = 0   UMETA(DisplayName = "None"),
	Hover = 1  UMETA(DisplayName = "Hover"),
};

// ----------------------------------------------------------------------------------
// 1. SAVED MOVE STRUCT 
// ----------------------------------------------------------------------------------
class FSavedMove_Hover : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

	uint8 bSavedWantsToBoost : 1;
	uint8 bSavedWantsToHop : 1;
	float SavedTargetYaw = 0.0f;

};

// ----------------------------------------------------------------------------------
// 2. NETWORK PREDICTION DATA CLIENT
// ----------------------------------------------------------------------------------
class FNetworkPredictionData_Client_Hover : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_Hover(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOVERMOVEMENT_API UHMCHoverMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	friend class FSavedMove_Hover;

public:
	UHMCHoverMovementComponent();


	// --- SUSPENSION & HOVER ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Physics")
	float TargetHoverHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Physics")
	float MaxTraceDistance = 450.0f;

	/** Yay sertligi */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Physics")
	float HoverSpringStrength = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Physics")
	float HoverDamping = 60.0f;

	// --- MOVEMENT & STEERING ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float MaxHoverSpeed = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float HoverBrakingDeceleration = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float DriftGrip = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float TurnRate = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float MaxBankAngle = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float BankInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Hover Movement")
	float SurfaceAlignInterpSpeed = 10.0f;

	// NITRO & HOP ABILITIES
	// ----------------------------------------------------------------------------------

	/** When pressed, how many times the speed cap will be multiplied (e.g., 2600 * 1.6 = 4160 cm/s!) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Abilities")
	float BoostMultiplier = 1.6f;

	/** The vertical jump force applied when pressing Space to jump over bumps/depressions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HMC|Abilities")
	float HopImpulse = 1100.0f;

	// --- GETTERS & HELPERS ---
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;

	UFUNCTION(BlueprintCallable, Category = "HMC|Movement")
	void SetHoverModeActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "HMC|Movement")
	bool IsHovering() const;

	UFUNCTION(BlueprintCallable, Category = "HMC|Abilities")
	void SetWantsToBoost(bool bBoost);

	UFUNCTION(BlueprintCallable, Category = "HMC|Abilities")
	void RequestHop();

	/** To get the visual rotation of the hover mesh */
	UFUNCTION(BlueprintPure, Category = "HMC|Movement")
	FRotator GetHoverMeshRotation() const { return VisualMeshRotation; }

	// --- NETWORK PREDICTION AND COMPRESSION OVERRIDES	 ---
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	// Variables to be compressed in network prediction (Saved Moves):
	uint8 bWantsToBoost : 1;
	uint8 bWantsToHop : 1;
	
	float ReplayTargetYaw = 0.f;

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void PhysHover(float deltaTime, int32 Iterations);

private:
	bool TraceGround(FHitResult& OutHitResult, float& OutGroundPitchDeg) const;

	float CurrentBankRoll = 0.0f;
	FRotator VisualMeshRotation = FRotator::ZeroRotator;
	
	float CumulativeSimTime = 0.0f;

};

