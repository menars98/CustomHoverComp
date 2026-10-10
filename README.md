# Network-Predicted Hovercraft Movement Component (HMC)

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-blue.svg?logo=unrealengine)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B%20%26%20Blueprints-orange.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A high-performance, latency-resilient, and fully network-predicted **Hovercraft & Traversal Movement Component** for Unreal Engine 5. 

Built by extending `UCharacterMovementComponent` (CMC), this framework solves the notoriously difficult problem of real-time multiplayer vehicle physics. It delivers deterministic spring-damper suspension, terrain-normal alignment, dynamic banking (roll tilt), drift mechanics, and custom input capabilities (Boost & Hop) under extreme latency conditions (**150ms–300ms ping + packet loss**) without rubberbanding or jitter.

---

## 🎮 Video Showcases & Latency Stress Tests

### 1. Average Network Conditions (Smooth Prediction & Drift)
https://github.com/user-attachments/assets/43d89a1b-4b81-46b2-9bb0-b83850c69cd6


### 2. Extreme Network Conditions (High Jitter & Packet Loss Stress Test)
https://github.com/user-attachments/assets/e45d5ee9-3f71-4efd-90e8-25a1359efef0

---

## ✨ Architectural Highlights & Key Features

- **Client-Side Prediction & Reconciliation (`FSavedMove_Character`):**  
  Client movements are timestamped, buffered, compressed, and deterministically replayed upon server corrections.
- **Bit-Level Flag Compression (`GetCompressedFlags`):**  
  Custom gameplay inputs (`bWantsToBoost`, `bWantsToHop`) are packed into single-byte bitfields (`FLAG_Custom_0`, `FLAG_Custom_1`) avoiding expensive RPC overhead.
- **4-Point Surface-Normal Suspension (Plane Fitting):**  
  Downwards multi-trace array (Front, Back, Left, Right) with `bTraceComplex = true` that fits a smooth ground plane. Eliminates derivative normal noise over complex terrain meshes.
- **Slope-Relative Damping (No Ballistic Crest Launches):**  
  Suspension damping is calculated relative to surface elevation velocity, preventing the hovercraft from violently launching into orbit over dune crests.
- **Decoupled Drift & Planar Velocity Integration:**  
  Separates forward thrust from lateral slip angle using deterministic damping, preventing numerical integration drift between Client and Server.
- **Network Smoothing Integration:**  
  Leverages CMC's built-in exponential mesh smoothing (`SmoothClientPosition`) via visual hierarchy parenting, eliminating visual jitter during server reconciliation.

---

## 🏛️ System Architecture

```text
                  [ Player Input (Enhanced Input) ]
                                 │
                                 ▼
                  [ AHMCHoverCharacter (Pawn) ]
                                 │
         ┌───────────────────────┴───────────────────────┐
         ▼                                               ▼
[ UHMCHoverMovementComponent ]                 [ Visual Mesh Layer ]
  ├── FSavedMove_Hover Buffer                    └── Parented to CharacterMesh0
  ├── Bitflag Compression                           (Inherits Exponential
  ├── PhysHover (4-Point Suspension)                 Client Smoothing)
  └── Slope-Relative Damping
         │
         ▼ (ServerMove RPC Stream)
   [ Dedicated / Listen Server ]
     └── UpdateFromCompressedFlags ──► Verification & Replay
     
````

## ⚙️ Physics Mathematical Model

### 1. Spring-Damper Suspension (Hooke's Law + Relative Damping)

```text
Force_Spring = (TargetHeight - GroundDistance) * SpringStrength**

Force_Damping = -(Velocity.Z - GroundElevationVelocity) * DampingStrength 
```

Where ground elevation velocity is derived dynamically from surface normal:

```text
GroundElevationVelocity = -(Normal.X * Velocity.X + Normal.Y * Velocity.Y) / max(Normal.Z, 0.5)
```

### 2. Banking & Roll Tilt

```text
DesiredRoll = SteeringInput * MaxBankAngle
```

## 🚀 Quick Start & Integration

### 1. Installation

Clone or copy the HoverMovement folder into your project's Plugins/ directory:
```text
git clone https://github.com/menars98/CustomHoverComp.git
```
### 2. Setting Up a Hovercraft Character

Inherit your character from AHMCHoverCharacter (or replace the CMC in C++ via ObjectInitializer):
```C++
AHMCHoverCharacter::AHMCHoverCharacter(const FObjectInitializer& ObjectInitializer)
    : ACharacter(ObjectInitializer.SetDefaultSubobjectClass<UHMCHoverMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
}
```

## 3. Key Configurable Parameters (Details Panel)
**Parameter	              Default	    Description**

```TargetHoverHeight```	```90.0 cm```	Target distance between center of mass and ground.

```HoverSpringStrength```	```1000.0```	Suspension stiffness (spring multiplier).

```HoverDamping	60.0```	```Critical damping``` coefficient to eliminate oscillations.

```MaxHoverSpeed```	```2600.0 cm/s```	Base cruising speed ceiling.

```BoostMultiplier```	```1.6x```	Velocity and acceleration multiplier when boosting.

```HopImpulse```	```1000.0 cm/s```	Vertical hop impulse to leap over terrain obstacles.

```DriftGrip```	```3.0```	Lateral friction factor (lower = looser drift).

```MaxBankAngle```	```20.0``` deg	Maximum roll tilt during steering.

## 🛠️ Tech Stack
**Engine: Unreal Engine 5.8**

**Core Systems: Extended UCharacterMovementComponent, FSavedMove_Character, FNetworkPredictionData_Client_Character**

**Language: Modern C++**

**Platforms: Windows / PC (Multiplayer Listen & Dedicated Server compatible)**
### 📄 License

**This project is open-source under the MIT License.**

### 👨‍💻 Author

Muhammet Enes Urlu
Portfolio: m-enesportfolio.netlify.app
GitHub: @menars98
