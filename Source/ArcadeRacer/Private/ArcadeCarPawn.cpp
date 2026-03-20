// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeCarPawn.h"

#include "DynamicMesh/DynamicMeshOverlay.h"
#include "Interfaces/IPluginManager.h"

//TODO: Refactor - Body component?

// Sets default values
AArcadeCarPawn::AArcadeCarPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bAsyncPhysicsTickEnabled = true;

	VehiclePhysicsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("VehiclePhysicsComponent"));
	SetRootComponent(VehiclePhysicsComponent);
	VehiclePhysicsComponent->SetSimulatePhysics(true);
	VehiclePhysicsComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VehiclePhysicsComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	VehiclePhysicsComponent->SetCollisionResponseToAllChannels(ECR_Block);
	VehiclePhysicsComponent->SetBoxExtent(FVector(130,75,32));
	VehiclePhysicsComponent->SetLineThickness(1.0f);
	VehiclePhysicsComponent->SetHiddenInGame(false);

	
	VehicleBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleBodyMesh"));
	VehicleBodyMesh->SetupAttachment(VehiclePhysicsComponent);
	VehicleBodyMesh->SetSimulatePhysics(false);
	VehicleBodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	//Wheels
	FL_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Front Left Wheel"));
	FR_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Front Right Wheel"));
	RL_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Rear Left Wheel"));
	RR_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Reaar Right Wheel"));
	
	
	FL_Wheel->SetupAttachment(VehiclePhysicsComponent);
	FR_Wheel->SetupAttachment(VehiclePhysicsComponent);
	RL_Wheel->SetupAttachment(VehiclePhysicsComponent);
	RR_Wheel->SetupAttachment(VehiclePhysicsComponent);
	
	//Tweak default brake bias
	RL_Wheel->WheelBrakeStrength *= 0.4f;
	RR_Wheel->WheelBrakeStrength *= 0.4f;

#pragma region //Camera
	/// CAMAERA SETUP ///
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 800.0f;
	
	//Enable spring arm follow lag
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->bEnableCameraRotationLag = true;
	SpringArmComponent->CameraLagMaxDistance = 220.0f;
	SpringArmComponent->CameraLagSpeed = 5.0f;
	SpringArmComponent->CameraRotationLagSpeed = 10.0f;
	
	//Sprint arm inherits
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = true;
	SpringArmComponent->bInheritRoll = false;
	
	//Camera Setup
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->FieldOfView = 110.0f;
	#pragma endregion
}

// Called when the game starts or when spawned
void AArcadeCarPawn::OnConstruction(const FTransform &Transform)
{
	Super::OnConstruction(Transform);
	
	WheelsArray.Empty();
	WheelsArray.Add(FL_Wheel);
	WheelsArray.Add(FR_Wheel);
	WheelsArray.Add(RR_Wheel);
	WheelsArray.Add(RL_Wheel);
	
	FlushPersistentDebugLines(GetWorld());
	for (int wheel = 0; wheel < WheelsArray.Num(); ++wheel)
	{
		if (WheelsArray[wheel] != nullptr)
		{
			const TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[wheel];
			currentWheel->WheelAnchorPoint = currentWheel->GetComponentLocation();
			currentWheel->WheelAnchorPoint.Z += currentWheel->SuspensionRestDistance;
			
			//Debug Anchor Point and wheel point
			if(bShowDebug)
			{
				//DrawDebugPoint(GetWorld(),currentWheel->WheelAnchorPoint, 5.0f, FColor::Yellow,true);
				//DrawDebugSphere(GetWorld(),currentWheel->GetComponentLocation(),currentWheel->WheelDiameter,16,FColor::Black,true);
			}

		}
	}
	
	for (TPair<FString, FGear>& gear : VehicleData.Gears)
	{
		if(gear.Value.GearUpRPM > VehicleData.MaxRPM) gear.Value.GearUpRPM = VehicleData.MaxRPM;
	}
}

void AArcadeCarPawn::BeginPlay()
{
	Super::BeginPlay();
	SetCurrentGear("1");
	UE_LOG(LogTemp,Warning,TEXT("Gear Ratio: %f"), GetCurrentGearRatio());
	_CurrentEngineRPM = VehicleData.IdleRPM;
}
	
// Called every frame
void AArcadeCarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateWheelAngle(DeltaTime);
	
	//_CurrentTorqueAtEngine = GetTorqueAtRPM(_CurrentEngineRPM);
    _CurrentVehicleSpeed = GetVehicleCurrentSpeed();
	
#if WITH_EDITOR
	//DEBUGGIN'
	if (bShowDebug)
	{
		for (int Wheel = 0; Wheel < WheelsArray.Num(); ++Wheel)
		{
			UWheelSceneComponent* currentWheel = WheelsArray[Wheel];
			DrawDebugCoordinateSystem(GetWorld(),currentWheel->GetWheelMeshLocation(),currentWheel->GetComponentRotation(),50.0f,false,-1,0,4);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1231,1.0f, FColor::Yellow, FString::Printf(TEXT("Current Speed: %f | Current Gear: %s Gear Ratio: %f | WheelRPM: %f | EngineRPM: %f"), _CurrentVehicleSpeed, *_CurrentGear, VehicleData.Gears[_CurrentGear].GearRatio, _AverageDrivenWheelsRPM, _CurrentEngineRPM));
		}
	}
#endif
}

#pragma region //values
void AArcadeCarPawn::SetTargetWheelAngle(float turnVal)
{
	_TargetTurnAngle = turnVal * VehicleData.MaxTurningAngle;
}

void AArcadeCarPawn::UpdateWheelAngle(float DeltaTime)
{
	_CurrentTurnAngle = FMath::Lerp(_CurrentTurnAngle, _TargetTurnAngle, VehicleData.TurningSpeed * DeltaTime);
	FL_Wheel->SetRelativeRotation(FRotator(0,_CurrentTurnAngle,0));
	FR_Wheel->SetRelativeRotation(FRotator(0,_CurrentTurnAngle,0));
}

float AArcadeCarPawn::GetVehicleCurrentSpeed() const
{
	return FVector::DotProduct(VehiclePhysicsComponent->GetForwardVector(), VehiclePhysicsComponent->GetComponentVelocity()); 
}

//GEARING
void AArcadeCarPawn::SetCurrentGear(FString gearName)
{
	if(VehicleData.Gears.Contains(gearName)) _CurrentGear = gearName;
	else UE_LOG(LogTemp,Warning,TEXT("Invalid Gear Given!"));
}

float AArcadeCarPawn::GetCurrentGearRatio() const
{
	return VehicleData.Gears[_CurrentGear].GearRatio;
}

void AArcadeCarPawn::SetAccelPedalPosition(float pedalVal)
{
	_Throttle = pedalVal;
}

void AArcadeCarPawn::SetBrakePedalPosition(float pedalVal)
{
	_BrakePedalPosition = pedalVal;
}
#pragma endregion


//////PHYSICS////
#pragma region //Physics

void AArcadeCarPawn::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{	
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);
	VehiclePhysicsComponent->AsyncPhysicsTickComponent(DeltaTime, SimTime);
	ReceiveAsyncPhysicsTick(DeltaTime, SimTime);

	CalculateDriveTrain(DeltaTime);
	
	//Update Wheel Components
	for (int Wheel = 0; Wheel < WheelsArray.Num(); ++Wheel)
	{		
		TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[Wheel];
		if (currentWheel != nullptr)
		{
			//UPDATE WHEEL VALUES
			UpdateCurrentWheelRotationalValues(currentWheel, DeltaTime);
			
			//Apply Long forces
			ApplyLongitudinalForces(currentWheel);
			
			//Suspension Forces
			ApplySuspensionForce(currentWheel);

			//Sliding and turning forces
			ApplyLateralForces(currentWheel, DeltaTime);
		}
	}

	//Calculate AverageWheelRPM
	switch (VehicleData.VehicleDriveType) {
	case EVehicleDriveType::AllWheelDrive:
		_AverageDrivenWheelsRPM = (FL_Wheel->WheelRPM + FR_Wheel->WheelRPM + RL_Wheel->WheelRPM + RR_Wheel->WheelRPM) / 4;
		break;
	case EVehicleDriveType::RearWheelDrive:
		_AverageDrivenWheelsRPM = (RL_Wheel->WheelRPM + RR_Wheel->WheelRPM) / 2;
		break;
	case EVehicleDriveType::FrontWheelDrive:
		_AverageDrivenWheelsRPM = (FL_Wheel->WheelRPM + FR_Wheel->WheelRPM) / 2;\
		break;
	default: ;
	}
}

#pragma region //SUSPENSION
void AArcadeCarPawn::ApplySuspensionForce(TObjectPtr<UWheelSceneComponent> Wheel) const
{
	FVector force = FVector::ZeroVector;
	
	//Get wheel position
	FVector wheelPos = Wheel->GetComponentLocation();
	
	//Get Spring direction
	FVector springDir = Wheel->GetUpVector();
	
	//Get wheel velocity
	FVector wheelVelocity = VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(wheelPos);
	
	//Set Start and End pos for ray trace
	FVector startLocation = wheelPos;
	FVector endLocation = (springDir * (Wheel->SuspensionRestDistance * -1)) + wheelPos;
	
#if WITH_EDITOR
	//Debug start and end pos
	DrawDebugSphere(GetWorld(),startLocation,8.0f,16,FColor::Green);
	DrawDebugSphere(GetWorld(),endLocation,8.0f,16,FColor::Red);
#endif
	
	FHitResult hitResult;
	FCollisionQueryParams collisionParams = FCollisionQueryParams();
	collisionParams.AddIgnoredActor(this);
	
	//Do Trace
	GetWorld()->LineTraceSingleByChannel(hitResult, startLocation, endLocation,ECollisionChannel::ECC_Visibility, collisionParams);
	if (hitResult.bBlockingHit) //Wheel grounded
	{
		//Current wheel is grounded
		Wheel->bIsGrounded = true;
		
		//Get distance to hit
		float hitDist = hitResult.Distance;
		
		//Current spring offset
		float springOffset = Wheel->SuspensionRestDistance - hitDist;
		
		//magnitude of velocity along spring direction
		float springMagnitude = FVector::DotProduct(springDir, wheelVelocity);
		
		force = ((springOffset * Wheel->SuspensionStrength) - (springMagnitude * Wheel->SuspensionDamping)) * springDir;
		//Apply force
		VehiclePhysicsComponent->AddForceAtLocation(force,wheelPos);
#if WITH_EDITOR
		//if (GEngine && bShowDebug) GEngine->AddOnScreenDebugMessage((uint64)Wheel->GetUniqueID(),-1, FColor::Green,FString::Printf(TEXT("Force %s applied to wheel %s"), *force.ToString(), *Wheel.GetName()));
#endif
		
		//Set wheel mesh location
		Wheel->SetWheelMeshLocation(hitResult.Location + (springDir * Wheel->WheelRadius));
		DrawDebugSphere(GetWorld(),Wheel->GetWheelMeshLocation(),Wheel->WheelRadius,16,FColor::Black);
	}
	else //Wheel in air
	{
		Wheel->bIsGrounded = false;
	}
}
#pragma endregion 

#pragma region ///STEERING AND SLIDING
void AArcadeCarPawn::ApplyLateralForces(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime) const
{
	FVector force;
	
	if (Wheel->bIsGrounded)
	{
		//Get Wheel Velocity
		const FVector wheelVelocity = VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(Wheel->GetComponentLocation());
	
		//Get current wheel sliding velocity
		const float slidingMagnitude = FVector::DotProduct(Wheel->GetRightVector(), wheelVelocity);
		//const float targetVelocity = -slidingMagnitude * Wheel->TyreGripCurve->GetFloatValue(slidingMagnitude / 100); //TODO: make tyreGrip query a grip curve based on slidingMagnitude
		const float targetVelocity = -slidingMagnitude * Wheel->TyreLateralGrip;
		const float targetForce = targetVelocity / DeltaTime;
		force = Wheel->GetRightVector() * targetForce * (Wheel->TyreMass/10);
	
		VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
	}
}
#pragma endregion

#pragma region //Longitudinal Force

//UPDATE WHEEL ANGULAR VELOCITY AND RPM VALUES
void AArcadeCarPawn::UpdateCurrentWheelRotationalValues(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime)
{
	Wheel->WheelRPM = Wheel->WheelAngularVelocity * (60 / (2 * UE_PI));
}

float AArcadeCarPawn::GetEngineTorqueAtRPM(float RPM) const
{
	const float T = VehicleData.TorqueCurve->GetFloatValue(FMath::Clamp(FMath::Abs(RPM / VehicleData.MaxRPM), 0.0f, 1.0f)) * VehicleData.MaxTorque;
	return T ;
}

void AArcadeCarPawn::CalculateDriveTrain(float DeltaTime)
{
	//Engine RPM Delta
	_CurrentEngineTorque = _Throttle * GetEngineTorqueAtRPM(_CurrentEngineRPM);
	
	if (_CurrentEngineRPM < VehicleData.IdleRPM)
	{
		const float idleT = (VehicleData.IdleRPM - _CurrentEngineTorque) * VehicleData.ReturnToIdleStrength;
		_CurrentEngineTorque += idleT;
	}
	
	const float rpmDelta = (_CurrentEngineTorque / VehicleData.EngineIntertia) + VehicleData.TransmissionCouplingScalar * ((_AverageDrivenWheelsRPM * GetCurrentGearRatio() * VehicleData.GearDifferential)- _CurrentEngineRPM) - VehicleData.EngineDrag * _CurrentEngineRPM;
	_CurrentEngineRPM += rpmDelta * DeltaTime;
	_CurrentEngineRPM = FMath::Clamp(_CurrentEngineRPM, 0.0f, VehicleData.MaxRPM);
	
	float currentGearing = GetCurrentGearRatio() * VehicleData.GearDifferential;
	
	//Update Wheel Components
	for (int Wheel = 0; Wheel < WheelsArray.Num(); ++Wheel)
	{		
		TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[Wheel];
		if (currentWheel != nullptr)
		{
			int drivenWheelsNum = VehicleData.VehicleDriveType == EVehicleDriveType::AllWheelDrive ? 4 : 2;
			
			//Calculate wheels angular velocity
			const float expectedAngularVelocity = (_CurrentEngineRPM / currentGearing) * (2.0f * PI / 60.0f); //What we expect the wheel speed to be based on current RPM
			const float currentAngularVelocity = currentWheel->WheelAngularVelocity;
			
			//Negative forces
			const float breakingForce = FMath::Clamp(currentWheel->WheelBrakeStrength * _BrakePedalPosition, -currentWheel->maxBrakingForce, currentWheel->maxBrakingForce); //TODO: Implement?
			
			//Spring strength
			float k = 10 * VehicleData.TorqueCurve->GetFloatValue((_CurrentEngineRPM/VehicleData.MaxRPM) * currentGearing);
			
			//Update Angular Velocity
			
			//Positive Forces
			float delta = k * (expectedAngularVelocity - currentAngularVelocity);
			
			//Resistive Forces
			delta -= currentWheel->RotationalDrag * currentAngularVelocity;
			
			currentWheel->WheelAngularVelocity += delta * DeltaTime;		
			
			//Set Wheel Torque 
			currentWheel->Torque = _CurrentEngineRPM * currentGearing / drivenWheelsNum; //TODO: Review
			
			if (GEngine) GEngine->AddOnScreenDebugMessage((uint64)currentWheel->GetUniqueID(),-1, FColor::Green, FString::Printf(TEXT("Target: %f Current: %f Delta: %f | Torque: %f"), expectedAngularVelocity, currentWheel->WheelAngularVelocity, delta, currentWheel->Torque));
		}
	}
}

float AArcadeCarPawn::CalculateLongitudinalGrip(TObjectPtr<UWheelSceneComponent> Wheel)
{
	// Calculate Slip Ratio
	float surfaceSpeed = Wheel->GetWheelAngularVelocity() * Wheel->WheelRadius;
	float slip = FMath::Abs((surfaceSpeed - _CurrentVehicleSpeed) / FMath::Max(FMath::Abs(_CurrentVehicleSpeed), 1.0f));
	
	//const float grip = FMath::Clamp(1.0f - FMath::Abs(slipRatio), 0.0f, 1.0f);
	float grip;
	
	if (slip < Wheel->peakSlip)
	{
		grip = slip / Wheel->peakSlip;
	}
	else
	{
		grip = FMath::Exp(-Wheel->GripFalloff * (slip - Wheel->peakSlip));
	}
	
	grip = FMath::Clamp(grip, 0.0f, 1.0f);
	if (GEngine) GEngine->AddOnScreenDebugMessage((uint64)Wheel->GetUniqueID(),-1,FColor::MakeRandomSeededColor(Wheel->GetUniqueID()), FString::Printf(TEXT("Wheel %s Surface Speed: %f, Slip: %f, Grip: %f"), *Wheel->GetName(), surfaceSpeed, slip, grip));
	Wheel->CurrentGrip = grip;
	return grip; //TODO: USE SOMEWHERE!
}

void AArcadeCarPawn::ApplyLongitudinalForces(TObjectPtr<UWheelSceneComponent> Wheel)
{
	if(Wheel == nullptr) return;
	float f = Wheel->Torque * VehicleData.EngineForceMultiplier;
	FVector force = f * Wheel->GetForwardVector();
	
	//Throttle
	if (_Throttle > 0)
	{
		switch (VehicleData.VehicleDriveType) {
		case EVehicleDriveType::FrontWheelDrive:
			if(Wheel == FL_Wheel || Wheel == FR_Wheel)
			{
				VehiclePhysicsComponent->AddImpulseAtLocation(force, Wheel->GetComponentLocation());
			}
			break;
		case EVehicleDriveType::RearWheelDrive:
			if(Wheel == RL_Wheel || Wheel == RR_Wheel)
			{
				VehiclePhysicsComponent->AddImpulseAtLocation(force, Wheel->GetComponentLocation());
			}
			break;
		case EVehicleDriveType::AllWheelDrive:
				VehiclePhysicsComponent->AddImpulseAtLocation(force, Wheel->GetComponentLocation());
			break;
		default: ;
		}
	}
}


#pragma endregion
#pragma endregion //End Physics Region

