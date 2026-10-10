#include "HMCHoverMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include <HMCHoverCharacter.h>

// ----------------------------------------------------------------------------------
// SAVED MOVE IMPLEMENTATION
// ----------------------------------------------------------------------------------

void FSavedMove_Hover::Clear()
{
	Super::Clear();

	bSavedWantsToBoost = 0;
	bSavedWantsToHop = 0;
	SavedTargetYaw = 0.0f;
}

uint8 FSavedMove_Hover::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	// Using Unreal's custom flag bits (FLAG_Custom_0 and FLAG_Custom_1):
	if (bSavedWantsToBoost)
	{
		Result |= FLAG_Custom_0; // 0x10 (4th bit)
	}

	if (bSavedWantsToHop)
	{
		Result |= FLAG_Custom_1; // 0x20 (5th bit)
	}

	return Result;
}

bool FSavedMove_Hover::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Hover* Other = static_cast<const FSavedMove_Hover*>(NewMove.Get());
	// Only combine if Boost or Hop state hasn't changed, otherwise don't combine:
	if (bSavedWantsToBoost != Other->bSavedWantsToBoost || bSavedWantsToHop != Other->bSavedWantsToHop)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Hover::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UHMCHoverMovementComponent* MoveComp = Cast<UHMCHoverMovementComponent>(C->GetCharacterMovement());
	if (MoveComp)
	{
		bSavedWantsToBoost = MoveComp->bWantsToBoost;
		bSavedWantsToHop = MoveComp->bWantsToHop;
		SavedTargetYaw = C->GetControlRotation().Yaw;
	}
}

void FSavedMove_Hover::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UHMCHoverMovementComponent* MoveComp = Cast<UHMCHoverMovementComponent>(C->GetCharacterMovement());
	if (MoveComp)
	{
		MoveComp->bWantsToBoost = bSavedWantsToBoost;
		MoveComp->bWantsToHop = bSavedWantsToHop;
		C->SetActorRotation(FRotator(0.0f, StartRotation.Yaw, 0.0f));
		MoveComp->ReplayTargetYaw = SavedTargetYaw;
	}
}

// ----------------------------------------------------------------------------------
// PREDICTION DATA CLIENT & ALLOCATE
// ----------------------------------------------------------------------------------

FNetworkPredictionData_Client_Hover::FNetworkPredictionData_Client_Hover(const UCharacterMovementComponent& ClientMovement)
	: FNetworkPredictionData_Client_Character(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Hover::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Hover());
}

FNetworkPredictionData_Client* UHMCHoverMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		UHMCHoverMovementComponent* MutableThis = const_cast<UHMCHoverMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Hover(*this);
	}

	return ClientPredictionData;
}

// ----------------------------------------------------------------------------------
// SERVER SIDE (DECOMPRESSING FLAGS)
// ----------------------------------------------------------------------------------

void UHMCHoverMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// Decompress the bits from the packet received from the client and apply them to the server-side variables:
	bWantsToBoost = ((Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
	bWantsToHop = ((Flags & FSavedMove_Character::FLAG_Custom_1) != 0);
}


// ----------------------------------------------------------------------------------
// HOVER MOVEMENT COMPONENT IMPLEMENTATION
// ----------------------------------------------------------------------------------
UHMCHoverMovementComponent::UHMCHoverMovementComponent()
{
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;

	DefaultLandMovementMode = MOVE_Custom;
	CustomMovementMode = static_cast<uint8>(EHMC_CustomMovementMode::Hover);

	bWantsToBoost = false;
	bWantsToHop = false;

	bServerAcceptClientAuthoritativePosition = true;
	bIgnoreClientMovementErrorChecksAndCorrection = true;

	bNetworkSmoothingComplete = false;
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

	NetworkSimulatedSmoothLocationTime = 0.15f; 
	NetworkSimulatedSmoothRotationTime = 0.15f; 

	ListenServerNetworkSimulatedSmoothLocationTime = 0.10f;
	ListenServerNetworkSimulatedSmoothRotationTime = 0.10f;

	NetworkMinTimeBetweenClientAdjustments = 0.20f;
	NetworkLargeClientCorrectionDistance = 45.0f;

}

float UHMCHoverMovementComponent::GetMaxSpeed() const
{
	if (IsHovering())
	{
		return bWantsToBoost ? (MaxHoverSpeed * BoostMultiplier) : MaxHoverSpeed;
	}
	return Super::GetMaxSpeed();
}

float UHMCHoverMovementComponent::GetMaxAcceleration() const
{
	const float BaseAcc = Super::GetMaxAcceleration();
	
	return (IsHovering() && bWantsToBoost) ? (BaseAcc * BoostMultiplier) : BaseAcc;
}

void UHMCHoverMovementComponent::SetHoverModeActive(bool bActive)
{
	if (bActive)
	{
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EHMC_CustomMovementMode::Hover));
	}
	else
	{
		SetMovementMode(MOVE_Walking);
	}
}

bool UHMCHoverMovementComponent::IsHovering() const
{
	// None or Hover custom modes
	return (MovementMode == MOVE_Custom &&
		(CustomMovementMode == static_cast<uint8>(EHMC_CustomMovementMode::Hover) || CustomMovementMode == 0));
}

void UHMCHoverMovementComponent::SetWantsToBoost(bool bBoost)
{
	bWantsToBoost = bBoost;
}

void UHMCHoverMovementComponent::RequestHop()
{
	bWantsToHop = true;
}

bool UHMCHoverMovementComponent::TraceGround(FHitResult& OutHitResult, float& OutGroundPitchDeg) const
{
	if (!CharacterOwner) return false;

	const FVector Center = CharacterOwner->GetActorLocation();
	const FVector Forward = CharacterOwner->GetActorForwardVector();
	const FVector Right = CharacterOwner->GetActorRightVector();

	// 4-POINT PLANE RAYS (Front, Back, Left, Right):
	const float BoxLength = 60.0f;
	const float BoxWidth = 30.0f;

	const FVector P_Front = Center + (Forward * BoxLength);
	const FVector P_Back = Center - (Forward * BoxLength);
	const FVector P_Right = Center + (Right * BoxWidth);
	const FVector P_Left = Center - (Right * BoxWidth);

	const FVector TraceOffset(0.0f, 0.0f, -MaxTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterOwner);
	QueryParams.bTraceComplex = true; // Trace the actual polygon surface!

	FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::InitType::AllStaticObjects);

	FHitResult HitF, HitB, HitR, HitL;
	const bool bF = GetWorld()->LineTraceSingleByObjectType(HitF, P_Front, P_Front + TraceOffset, ObjectParams, QueryParams);
	const bool bB = GetWorld()->LineTraceSingleByObjectType(HitB, P_Back, P_Back + TraceOffset, ObjectParams, QueryParams);
	const bool bR = GetWorld()->LineTraceSingleByObjectType(HitR, P_Right, P_Right + TraceOffset, ObjectParams, QueryParams);
	const bool bL = GetWorld()->LineTraceSingleByObjectType(HitL, P_Left, P_Left + TraceOffset, ObjectParams, QueryParams);

	// If at least 3 points hit, fit a stable plane:
	if (bF && bB && bR && bL)
	{
		OutHitResult = HitF;
		OutHitResult.Distance = (HitF.Distance + HitB.Distance + HitR.Distance + HitL.Distance) * 0.25f;

		// Calculate the noise-free, filtered surface normal derived from 4 points:
		const FVector Longitudinal = (HitF.ImpactPoint - HitB.ImpactPoint).GetSafeNormal();
		const FVector Lateral = (HitR.ImpactPoint - HitL.ImpactPoint).GetSafeNormal();
		const FVector FittedNormal = FVector::CrossProduct(Lateral, Longitudinal).GetSafeNormal();

		OutHitResult.ImpactNormal = (FittedNormal.Z > 0.0f) ? FittedNormal : -FittedNormal;

		// Nose pitch:
		const float DeltaZ = HitF.ImpactPoint.Z - HitB.ImpactPoint.Z;
		const float HorizDist = FVector::Dist2D(HitF.ImpactPoint, HitB.ImpactPoint);
		OutGroundPitchDeg = FMath::RadiansToDegrees(FMath::Atan2(DeltaZ, FMath::Max(HorizDist, 1.0f)));

		return true;
	}

	// Fallback single contacts:
	if (bF || bB)
	{
		OutHitResult = bF ? HitF : HitB;
		OutGroundPitchDeg = 0.0f;
		return true;
	}

	OutGroundPitchDeg = 0.0f;
	return false;
}

void UHMCHoverMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (IsHovering())
	{
		PhysHover(deltaTime, Iterations);
	}
}

void UHMCHoverMovementComponent::PhysHover(float deltaTime, int32 Iterations)
{

	if (deltaTime < MIN_TICK_TIME || !CharacterOwner) return;

	CumulativeSimTime += deltaTime;

	// 1. First, we determine the actor's rotation based on control rotation or replay target yaw:
	FRotator ActorRotation = CharacterOwner->GetActorRotation();
	if (CharacterOwner->GetController())
	{
		ActorRotation.Yaw = CharacterOwner->bClientUpdating
			? ReplayTargetYaw
			: CharacterOwner->GetControlRotation().Yaw;
	}
	
	// Forward and Right vectors based on the CURRENT Yaw:
	const FRotator YawOnlyRot(0.0f, ActorRotation.Yaw, 0.0f);
	const FVector FlatForward = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::Y);

	// 2. GROUND AND ANALYTIC NORMAL BASED SUSPENSION
	FHitResult GroundHit;
	float GroundPitchDeg = 0.0f;
	const bool bGrounded = TraceGround(GroundHit, GroundPitchDeg);

	FVector SpringAcceleration = FVector::ZeroVector;
	float TargetPitch = 0.0f;

	if (bGrounded)
	{
		const float Compression = TargetHoverHeight - GroundHit.Distance;
		const float SpringForce = Compression * HoverSpringStrength;

		// Calculate the ground's vertical velocity component based on the surface normal and current velocity:
		const FVector N = GroundHit.ImpactNormal;
		const float GroundVZ = (N.Z > 0.5f)
			? -(N.X * Velocity.X + N.Y * Velocity.Y) / N.Z
			: 0.0f;

		// Safe damping to prevent over-correction:
		const float RelVZ = Velocity.Z - GroundVZ;
		float DampingForce = -RelVZ * HoverDamping;

		// Limit damping to prevent it from being stronger than the spring at low FPS:
		const float MaxAllowedDamp = FMath::Abs(RelVZ) / FMath::Max(deltaTime, 0.001f);
		DampingForce = FMath::Clamp(DampingForce, -MaxAllowedDamp, MaxAllowedDamp);

		// Clamp the linear spring force to ±10000 to allow ±10 cm of linear deflection:
		SpringAcceleration.Z = FMath::Clamp(SpringForce + DampingForce, -10000.0f, 10000.0f);
		TargetPitch = GroundPitchDeg;

		const bool bIsReplay = CharacterOwner->bClientUpdating;
		if (Velocity.SizeSquared() > 1000.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] SimTime: %.4f | dt: %.4f | Loc: %s | Vel: %s | Yaw: %.2f | Accel: %s"),
			CharacterOwner->HasAuthority() ? TEXT("SRV") : TEXT("CLI"),
			CumulativeSimTime,
			deltaTime,
			*CharacterOwner->GetActorLocation().ToCompactString(),
			*Velocity.ToCompactString(),
			CharacterOwner->GetActorRotation().Yaw,
			*Acceleration.ToCompactString()
		);
		}
	}

	else
	{
		SpringAcceleration.Z = GetGravityZ();
		TargetPitch = 0.0f;
	}

	// 3. DIRECTION, TURNING BANK (ROLL)
	const FVector LocalAccel = CharacterOwner->GetActorTransform().InverseTransformVectorNoScale(Acceleration);
	const float MaxAcc = GetMaxAcceleration() > 0.0f ? GetMaxAcceleration() : 1.0f;
	const float SteeringInput = FMath::Clamp(LocalAccel.Y / MaxAcc, -1.0f, 1.0f);

	const float DesiredRoll = SteeringInput * MaxBankAngle;
	CurrentBankRoll = FMath::FInterpTo(CurrentBankRoll, DesiredRoll, deltaTime, BankInterpSpeed);

	VisualMeshRotation.Pitch = FMath::FInterpTo(VisualMeshRotation.Pitch, TargetPitch, deltaTime, SurfaceAlignInterpSpeed);
	VisualMeshRotation.Roll = CurrentBankRoll;
	VisualMeshRotation.Yaw = 0.0f;

	// 4. HOP (JUMP)
	if (bWantsToHop)
	{
		if (bGrounded)
		{
			Velocity.Z = HopImpulse;
		}
		bWantsToHop = false;
	}

	// 5. PLANAR VELOCITY & EXPONENTIAL DRIFT
	const float SavedZ = Velocity.Z;
	Velocity.Z = 0.0f;

	CalcVelocity(deltaTime, 0.0f, false, HoverBrakingDeceleration);

	float ForwardSpeed = FVector::DotProduct(Velocity, FlatForward);
	float LateralSpeed = FVector::DotProduct(Velocity, RightDir);

	// Apply exponential decay to lateral speed to simulate drift reduction:
	//LateralSpeed *= FMath::Exp(-DriftGrip * deltaTime);
	// Change of plan: Use FInterpTo for smoother interpolation instead of exponential decay:
	//LateralSpeed = FMath::FInterpTo(LateralSpeed, 0.0f, deltaTime, DriftGrip);
	const float DriftDecay = FMath::Clamp(1.0f - (DriftGrip * deltaTime), 0.0f, 1.0f);
	LateralSpeed *= DriftDecay;

	Velocity.X = (FlatForward.X * ForwardSpeed) + (RightDir.X * LateralSpeed);
	Velocity.Y = (FlatForward.Y * ForwardSpeed) + (RightDir.Y * LateralSpeed);
	Velocity.Z = SavedZ + (SpringAcceleration.Z * deltaTime);

	// 6. MOTOR MOVEMENT AND SAFE COLLISION
	const FVector MoveDelta = Velocity * deltaTime;
	const FRotator CapsuleRotation(0.0f, ActorRotation.Yaw, 0.0f);

	FHitResult MoveHit;
	SafeMoveUpdatedComponent(MoveDelta, CapsuleRotation, true, MoveHit);

	if (MoveHit.IsValidBlockingHit())
	{
		const FVector HitNormal = MoveHit.Normal;
		SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, HitNormal, MoveHit, true);
		Velocity = FVector::VectorPlaneProject(Velocity, HitNormal);
	}
}