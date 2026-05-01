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
	SetCurrentGear("neutral");
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
			GEngine->AddOnScreenDebugMessage(1231,1.0f, FColor::Yellow, FString::Printf(TEXT("Current Speed: %f | Current Gear: %s Gear Ratio: %f | EngineRPM: %f"), _CurrentVehicleSpeed, *_CurrentGear, VehicleData.Gears[_CurrentGear].GearRatio, _CurrentEngineRPM));
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
			
			//Suspension Forces
			ApplySuspensionForce(currentWheel);

			//Sliding and turning forces
			ApplyLateralForces(currentWheel, DeltaTime);
		}
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

void AArcadeCarPawn::CalculateDriveTrain(float DeltaTime)
{
	//Calculate current torque at engine
	const float rpmPercentile = _CurrentEngineRPM / VehicleData.MaxRPM;
	_CurrentEngineTorque = VehicleData.TorqueCurve->GetFloatValue(rpmPercentile) * VehicleData.MaxTorque;

	//Calculate Engine RPM Actual
	float ExpectedRPMFromWheels = (_AverageDrivenWheelsAngularVelocity * 60 / (UE_PI * 2)) * GetCurrentGearRatio() * VehicleData.GearDifferential; //The RPM the wheels expect the engine to be at
	//UE_LOG(LogTemp, Warning,TEXT("ERPM: %f"), ExpectedRPMFromWheels);
	
	float TargetRPMFreeRev = _Throttle * VehicleData.MaxRPM;
	
	float RPMGains = _CurrentEngineTorque * _Throttle;
	
	float RPMDifference = VehicleData.EngineFriction * (GetCurrentGearRatio() !=0 ? ExpectedRPMFromWheels : TargetRPMFreeRev - _CurrentEngineRPM);
	float RPMLosses = VehicleData.EngineBreaking * _CurrentEngineRPM;

	//Apply Engine RPM
	float EngineRPMDelta = RPMGains + RPMDifference - RPMLosses;
	_CurrentEngineRPM += EngineRPMDelta * DeltaTime;

	if (_CurrentEngineRPM < VehicleData.IdleRPM)
	{
		float delta = (VehicleData.IdleRPM - _CurrentEngineRPM) + (_CurrentEngineTorque * VehicleData.ReturnToIdleStrength);
		_CurrentEngineRPM += delta * DeltaTime;
	}

	_CurrentEngineRPM = FMath::Clamp(_CurrentEngineRPM, 0, VehicleData.MaxRPM);
	TryRotateWheels(DeltaTime);
}

void AArcadeCarPawn::TryRotateWheels(float DeltaTime)
{
	//Do drivetrain
	const float Gearing  = GetCurrentGearRatio() * VehicleData.GearDifferential;
	const int NumOfDrivenWheels = VehicleData.VehicleDriveType == EVehicleDriveType::AllWheelDrive ? 4 : 2;
	
	//Calculate Torque at wheels
	float wheelTorque = (_CurrentEngineTorque * Gearing) / NumOfDrivenWheels;
	
	
	for (int Wheel = 0; Wheel < WheelsArray.Num(); ++Wheel)
	{
		
		TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[Wheel];
		if (currentWheel != nullptr)
		{
			//Calculate Current Angular Velocity in physics
			float lVelocity = FVector::DotProduct(currentWheel->GetForwardVector(), VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(currentWheel->GetComponentLocation()));
			float AngleVel = lVelocity / (currentWheel->WheelRadius / 10);
			currentWheel->WheelAngularVelocity = AngleVel;
			
			//Calculate RPM from angular velocity
			currentWheel->WheelRPM = currentWheel->WheelAngularVelocity * (60 / (2 * UE_PI)); //Calculate wheel RPM based on angle velocity
			
			//Acelleration force
			switch (VehicleData.VehicleDriveType)
			{
			case EVehicleDriveType::FrontWheelDrive:
				if(currentWheel == FL_Wheel || currentWheel == FR_Wheel)
				{
					//Apply force from engine 
					FVector force = (wheelTorque / (currentWheel->WheelRadius / 100)) * currentWheel->GetForwardVector();
					VehiclePhysicsComponent->AddForceAtLocation(force, currentWheel->GetComponentLocation());
					
					_AverageDrivenWheelsAngularVelocity = (FL_Wheel->WheelAngularVelocity + FR_Wheel->WheelAngularVelocity) / NumOfDrivenWheels;
				}
				break;
			case EVehicleDriveType::RearWheelDrive:
				{
					//Apply force from engine 
					FVector force = (wheelTorque / (currentWheel->WheelRadius / 100)) * currentWheel->GetForwardVector();
					VehiclePhysicsComponent->AddForceAtLocation(force, currentWheel->GetComponentLocation());
					
					_AverageDrivenWheelsAngularVelocity = (RL_Wheel->WheelAngularVelocity + RR_Wheel->WheelAngularVelocity) / NumOfDrivenWheels;
				}
				break;
			case EVehicleDriveType::AllWheelDrive:
				{
					//Apply force from engine 
					FVector force = (wheelTorque / (currentWheel->WheelRadius / 100)) * currentWheel->GetForwardVector();
					VehiclePhysicsComponent->AddForceAtLocation(force, currentWheel->GetComponentLocation());
					
					_AverageDrivenWheelsAngularVelocity = (FL_Wheel->WheelAngularVelocity + FR_Wheel->WheelAngularVelocity + RL_Wheel->WheelAngularVelocity + RR_Wheel->WheelAngularVelocity) / NumOfDrivenWheels;
				}
				break;
			}
			
			//Decelleration Force
			switch (VehicleData.VehicleDriveType)
			{
			case EVehicleDriveType::FrontWheelDrive:
				if(currentWheel == FL_Wheel || currentWheel == FR_Wheel)
				{
					FVector force = (_BrakePedalPosition * currentWheel->WheelBrakeStrength) * -currentWheel->GetForwardVector();
					VehiclePhysicsComponent->AddForceAtLocation(force, currentWheel->GetComponentLocation());
				}
				break;
			}
		}
	}
}

#pragma endregion
#pragma endregion //End Physics Region

