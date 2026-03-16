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
	UpdateCurrentRPM(DeltaTime);
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
			GEngine->AddOnScreenDebugMessage(1231,-1.0f, FColor::Green, FString::Printf(TEXT("Current Speed: %f Accel Pedal Position: %f"), _CurrentVehicleSpeed, _AcceleratorPedalPosition));
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

void AArcadeCarPawn::UpdateCurrentRPM(float DeltaTime)
{
	float RPMPercentile = FMath::Clamp(FMath::Abs(_CurrentRPM / VehicleData.MaxRPM), 0.0f, 1.0f);
	float RevSpeed = VehicleData.RevUpSpeed * (VehicleData.TorqueCurve != nullptr ? VehicleData.TorqueCurve->GetFloatValue(RPMPercentile) : 1.0f);
	
	if (_AcceleratorPedalPosition > 0)
	{
		if (_CurrentRPM < VehicleData.MaxRPM)
		{
			//Increase RPM
			_CurrentRPM += DeltaTime * RevSpeed;
		}
		else
		{
			//Limit to max
			_CurrentRPM = VehicleData.MaxRPM;
		}
	}
	else
	{
		//TODO: Revhang?
		if (_CurrentRPM > VehicleData.IdleRPM)
		{
			//Decrease RPM
			_CurrentRPM -= DeltaTime * VehicleData.RevDownSpeed;	
		}
		else _CurrentRPM = VehicleData.IdleRPM;
	}
#if WITH_EDITOR
	if (GEngine) GEngine->AddOnScreenDebugMessage(213123, -1,FColor::Green, FString::Printf(TEXT("Current RPM: %f Current Torque: %f RevSpeed: %f"), _CurrentRPM, GetTorqueAtRPM(_CurrentRPM), RevSpeed));
#endif
}

float AArcadeCarPawn::GetVehicleCurrentSpeed() const
{
	return FVector::DotProduct(VehiclePhysicsComponent->GetForwardVector(), VehiclePhysicsComponent->GetComponentVelocity());
}

float AArcadeCarPawn::GetCurrentGearRatio() const
{
	return _CurrentGear.GearRatio / 1;
}

float AArcadeCarPawn::GetTorqueAtRPM(float RPM) const
{
	float RPMPercentile = FMath::Clamp(FMath::Abs(RPM / VehicleData.MaxRPM), 0.0f, 1.0f);
	if (VehicleData.TorqueCurve != nullptr) return VehicleData.TorqueCurve->GetFloatValue(RPMPercentile) * VehicleData.MaxTorque;
	else return 0.0f;
}

void AArcadeCarPawn::SetAccelPedalPosition(float pedalVal)
{
	_AcceleratorPedalPosition = pedalVal;
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
		TObjectPtr<UWheelSceneComponent> currentWheel = WheelsArray[Wheel];
		if (currentWheel != nullptr)
		{
			ApplySuspensionForce(currentWheel);
			ApplyLateralForces(currentWheel, DeltaTime);
			//ApplyAccelerationForces(currentWheel, DeltaTime);
			CalculateAccelerationForces(currentWheel);
		}
	}
}

//SUSPENSION
FVector AArcadeCarPawn::ApplySuspensionForce(TObjectPtr<UWheelSceneComponent> Wheel)
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
	return force;
}

///STEERING AND SLIDING
FVector AArcadeCarPawn::ApplyLateralForces(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime)
{
	FVector force = FVector::ZeroVector;
	
	if (Wheel->bIsGrounded)
	{
		//Get Wheel Velocity
		FVector wheelVelocity = VehiclePhysicsComponent->GetPhysicsLinearVelocityAtPoint(Wheel->GetComponentLocation());
	
		//Get current wheel sliding velocity
		float slidingMagnitude = FVector::DotProduct(Wheel->GetRightVector(), wheelVelocity);
		float targetVelocity = -slidingMagnitude * Wheel->TyreGrip; //TODO: make tyreGrip query a grip curve based on slidingMagnitude
		float targetForce = targetVelocity / DeltaTime;
		force = Wheel->GetRightVector() * targetForce * (Wheel->TyreMass/10);
	
		VehiclePhysicsComponent->AddForceAtLocation(force, Wheel->GetComponentLocation());
		return force;
	}
	else return force;
}

void AArcadeCarPawn::CalculateAccelerationForces(TObjectPtr<UWheelSceneComponent> Wheel)
{
	FVector force = FVector::ZeroVector;
	_CurrentVehicleSpeed = GetVehicleCurrentSpeed();
	if (VehicleData.TorqueCurve != nullptr)
	{
		if (Wheel->bIsGrounded)
		{
			//wee
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


//TODO: Gearing?
//ACCEL AND BRAKING

#pragma endregion

