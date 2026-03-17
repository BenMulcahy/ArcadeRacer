// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeCarPawn.h"

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
	Fl_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Front Left Wheel"));
	FR_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Front Right Wheel"));
	RL_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Rear Left Wheel"));
	RR_Wheel = CreateDefaultSubobject<UWheelSceneComponent>(TEXT("Reaar Right Wheel"));
	
	
	Fl_Wheel->SetupAttachment(VehiclePhysicsComponent);
	FR_Wheel->SetupAttachment(VehiclePhysicsComponent);
	RL_Wheel->SetupAttachment(VehiclePhysicsComponent);
	RR_Wheel->SetupAttachment(VehiclePhysicsComponent);


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
	WheelsArray.Add(Fl_Wheel);
	WheelsArray.Add(FR_Wheel);
	WheelsArray.Add(RR_Wheel);
	WheelsArray.Add(RL_Wheel);
	
	FlushPersistentDebugLines(GetWorld());
	for (int wheel = 0; wheel < WheelsArray.Num(); ++wheel)
	{
		if (WheelsArray[wheel] != nullptr)
		{
			TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[wheel];
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

	for (int gear = 0; gear < GearsArray.Num(); ++gear)
	{
		if(GearsArray[gear].GearUpRPM > VehicleData.MaxRPM) GearsArray[gear].GearUpRPM = VehicleData.MaxRPM; //Ensure cant gear up past gear RPM
	}
}

void AArcadeCarPawn::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp,Warning,TEXT("Gear Ratio: %f"), GetCurrentGearRatio());
}
	
// Called every frame
void AArcadeCarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateWheelAngle(DeltaTime);
	UpdateCurrentEngineRPM(DeltaTime);
	_CurrentTorque = GetTorqueAtRPM(_CurrentRPM);
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
			GEngine->AddOnScreenDebugMessage(1231,-1.0f, FColor::Green, FString::Printf(TEXT("Current Speed: %f Accel Pedal Position: %f"), _CurrentVehicleSpeed, _Throttle));
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
	Fl_Wheel->SetRelativeRotation(FRotator(0,_CurrentTurnAngle,0));
	FR_Wheel->SetRelativeRotation(FRotator(0,_CurrentTurnAngle,0));
}

void AArcadeCarPawn::UpdateCurrentEngineRPM(float DeltaTime)
{
	//Calculate RPM differential
	//_CurrentRPM += FMath::Clamp(TCValue + /*EngineBreaking*/ - EngineFriction, VehicleData.IdleRPM, VehicleData.MaxRPM) * DeltaTime;
	const float torque = VehicleData.TorqueCurve->GetFloatValue(_Throttle) * VehicleData.MaxRPM * GetCurrentGearRatio();
	_CurrentRPM += (torque * _Throttle - (EngineFriction * _CurrentRPM)) * DeltaTime;
	_CurrentRPM = FMath::Clamp(_CurrentRPM, VehicleData.IdleRPM, VehicleData.MaxRPM);

	//if(_CurrentGearNumber != GearsArray.Num()-1 && _CurrentRPM >= GearsArray[_CurrentGearNumber].GearUpRPM) GearUp(); //TODO: Make bool for automatic/manaul
	
#if WITH_EDITOR
	if (GEngine) GEngine->AddOnScreenDebugMessage(213123, -1,FColor::Green, FString::Printf(TEXT("Current RPM: %f Current Torque: %f Current Gear: %d"), _CurrentRPM, GetTorqueAtRPM(_CurrentRPM), _CurrentGearNumber));
#endif
}

void AArcadeCarPawn::UpdateCurrentWheelVelocity(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime)
{
	Wheel->WheelAngularVelocity  = FVector::DotProduct(VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(Wheel->GetComponentLocation()), Wheel->GetForwardVector()) / Wheel->WheelRadius;
	Wheel->WheelRPM = Wheel->GetWheelAngularVelocity() * (60 / (2 * UE_PI));

	#if WITH_EDITOR
	if (GEngine && bShowDebug) GEngine->AddOnScreenDebugMessage((uint64)Wheel->GetUniqueID(),-1, FColor::Green,FString::Printf(TEXT("Wheel Velocity %f | Wheel RPM: %f at wheel %s"), Wheel->GetWheelAngularVelocity() ,Wheel->WheelRPM, *Wheel.GetName()));
	#endif
}


float AArcadeCarPawn::GetVehicleCurrentSpeed() const
{
	return FVector::DotProduct(VehiclePhysicsComponent->GetForwardVector(), VehiclePhysicsComponent->GetComponentVelocity());
}

//GEARING
float AArcadeCarPawn::GetCurrentGearRatio() const
{
	return GearsArray[_CurrentGearNumber].GearRatio / 1;
}

void AArcadeCarPawn::GearUp()
{
	if(_CurrentGearNumber < GearsArray.Num()-1) _CurrentGearNumber++;
	//SetCurrentRPM(GearsArray[_CurrentGearNumber].GearDownRPM);
}

void AArcadeCarPawn::GearDown()
{
	if(_CurrentGearNumber > 0) _CurrentGearNumber--; //TODO: REVERSE?
	SetCurrentRPM(GearsArray[_CurrentGearNumber].GearUpRPM);
}

float AArcadeCarPawn::GetTorqueAtRPM(float RPM) const
{
	const float RPMPercentile = FMath::Clamp(FMath::Abs(RPM / VehicleData.MaxRPM), 0.0f, 1.0f);
	if (VehicleData.TorqueCurve != nullptr) return VehicleData.TorqueCurve->GetFloatValue(RPMPercentile) * VehicleData.MaxTorque;
	else return 0.0f;
}

void AArcadeCarPawn::SetCurrentRPM(float RPM)
{
	_CurrentRPM = RPM;
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

#pragma region //Physics
void AArcadeCarPawn::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);
	VehiclePhysicsComponent->AsyncPhysicsTickComponent(DeltaTime, SimTime);
	ReceiveAsyncPhysicsTick(DeltaTime, SimTime);

	//Update Wheel Components
	for (int Wheel = 0; Wheel < WheelsArray.Num(); ++Wheel)
	{
		//Calculate Engine Forces
		//GetForceAtWheels();
		
		TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[Wheel];
		if (currentWheel != nullptr)
		{
			//Calculate Suspension Forces
			ApplySuspensionForce(currentWheel);

			//Calculate Sliding and turning forces
			ApplyLateralForces(currentWheel, DeltaTime);
			
			//Force At wheel * wheel rad = force?
			ApplyAccelerationForcesAtWheel(currentWheel);

			UpdateCurrentWheelVelocity(currentWheel, DeltaTime);
		}
	}
}

///ACCELERATION
void AArcadeCarPawn::ApplyAccelerationForcesAtWheel(TObjectPtr<UWheelSceneComponent> Wheel)
{
	FVector force = FVector::ZeroVector;
	if (_Throttle > 0)
	{
		switch (VehicleData.VehicleDriveType)
		{
		case EVehicleDriveType::FrontWheelDrive:
			if (Wheel == Fl_Wheel || Wheel == FR_Wheel)
			{
				force = Wheel->GetForwardVector() * (_CurrentTorque / 2) * Wheel->WheelRadius * GetCurrentGearRatio();
				VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
			}
			break;
		case EVehicleDriveType::RearWheelDrive:
			if (Wheel == RL_Wheel || Wheel == RR_Wheel)
			{
				force = Wheel->GetForwardVector() * (_CurrentTorque / 2) * Wheel->WheelRadius * GetCurrentGearRatio();
				VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
			}
			break;
		case EVehicleDriveType::AllWheelDrive:
			force = Wheel->GetForwardVector() * (_CurrentTorque / 4) * Wheel->WheelRadius * GetCurrentGearRatio();
			VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
			break;
		}
		if(force != FVector::Zero()) DrawDebugDirectionalArrow(GetWorld(),Wheel->GetComponentLocation(),Wheel->GetComponentLocation() + force,5.0f,FColor::Blue);
	}
}

//SUSPENSION
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

///STEERING AND SLIDING
void AArcadeCarPawn::ApplyLateralForces(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime) const
{
	FVector force = FVector::ZeroVector;
	
	if (Wheel->bIsGrounded)
	{
		//Get Wheel Velocity
		const FVector wheelVelocity = VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(Wheel->GetComponentLocation());
	
		//Get current wheel sliding velocity
		const float slidingMagnitude = FVector::DotProduct(Wheel->GetRightVector(), wheelVelocity);
		const float targetVelocity = -slidingMagnitude * Wheel->TyreGrip; //TODO: make tyreGrip query a grip curve based on slidingMagnitude
		const float targetForce = targetVelocity / DeltaTime;
		force = Wheel->GetRightVector() * targetForce * (Wheel->TyreMass/10);
	
		VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
	}
}


//Graveyard
#pragma region
/*

void AArcadeCarPawn::GetForceAtWheels()
{
	//Lookup base torque
	float torque = GetTorqueAtRPM(_CurrentRPM);

	//Pass through transmission
	
}

void AArcadeCarPawn::CalculateAccelerationForces(TObjectPtr<UWheelSceneComponent> Wheel)
{
	FVector force = FVector::ZeroVector;
	_CurrentVehicleSpeed = GetVehicleCurrentSpeed();
	if (VehicleData.TorqueCurve != nullptr)
	{
		if (Wheel->bIsGrounded)
		{
			//TODO: Look at 2D platformer controller for inspo on forces calcs? -> Max speed/Gear maybe instead of rpm idk
			if (_AcceleratorPedalPosition > 0)
			{
				switch (VehicleData.VehicleDriveType)
				{
				case EVehicleDriveType::FrontWheelDrive:
					if (Wheel == Fl_Wheel || Wheel == FR_Wheel)
					{
						force = Wheel->GetForwardVector() * _CurrentTorque * 10.0f * GetCurrentGearRatio();
						VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
					}
					break;
				case EVehicleDriveType::RearWheelDrive:
					if (Wheel == RL_Wheel || Wheel == RR_Wheel)
					{
						force = Wheel->GetForwardVector() * _CurrentTorque * 10.0f * GetCurrentGearRatio();
						VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
					}
					break;
				case EVehicleDriveType::AllWheelDrive:
					force = Wheel->GetForwardVector() * _CurrentTorque * 10.0f * GetCurrentGearRatio();
					VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
					break;
				}
			}
		}
	}
}
*/
#pragma endregion

//TODO: Gearing?
//ACCEL AND BRAKING

#pragma endregion

