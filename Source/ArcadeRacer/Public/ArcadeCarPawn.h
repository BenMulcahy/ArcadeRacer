// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "WheelSceneComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ArcadeCarPawn.generated.h"


UENUM(BlueprintType)
enum class EVehicleDriveType : uint8
{
	FrontWheelDrive,
	RearWheelDrive,
	AllWheelDrive,
};

USTRUCT()
struct FGear
{
	GENERATED_BODY()
	int32 GearNumber = 1;
	//EG GearRatio:1
	int32 GearRatio = 3;
};

USTRUCT(BlueprintType)
struct FVehicleData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector VehicleCOM;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EVehicleDriveType VehicleDriveType = EVehicleDriveType::FrontWheelDrive;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxTurningAngle = 30.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TurningSpeed = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxSpeed = 180.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 GearUpRPM = 6800;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RevUpSpeed = 4000.0f; //Rev/S
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RevDownSpeed = 4200.0f; //Rev/S
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> TorqueCurve;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxPower = 200.0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxTorque = 180.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MaxRPM = 7800;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 IdleRPM = 1200;
};

UCLASS()
class ARCADERACER_API AArcadeCarPawn : public APawn
{
	GENERATED_BODY()

//Vars
public:

	// Sets default values for this pawn's properties
	AArcadeCarPawn();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle Data | DEBUGGING")
	bool bShowDebug = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle Data")
	FVehicleData VehicleData;
	
	//Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	//Camera Spring Arm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	//Vehicle Mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Physics Mesh")
	TObjectPtr<UStaticMeshComponent> VehicleBodyMesh = nullptr;
	
	//VehiclePhysics
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Physics Mesh")
	TObjectPtr<UBoxComponent> VehiclePhysicsComponent = nullptr;

	//Wheels
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Wheels")
	TObjectPtr<UWheelSceneComponent> Fl_Wheel;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Wheels")
	TObjectPtr<UWheelSceneComponent>  FR_Wheel;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Wheels")
	TObjectPtr<UWheelSceneComponent>  RL_Wheel;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Wheels")
	TObjectPtr<UWheelSceneComponent>  RR_Wheel;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Data | Wheels")
	TArray<TObjectPtr<UWheelSceneComponent>> WheelsArray;

private:
	float _CurrentTurnAngle = 0.0f;
	float _TargetTurnAngle = 0.0f;
	float _CurrentVehicleSpeed = 0.0f;
	float _SpeedPercentile = 0.0f;
	
	//0-1 value of where accel 'pedal' is
	float _AcceleratorPedalPosition = 0.0f;
	//where brake 'pedal' is
	float _BrakePedalPosition = 0.0f;
	
	float _CurrentRPM = 0.0f;
	float _CurrentTorque = 0.0f;
	FGear _CurrentGear = FGear(1,3);
	
//Functions
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//Async Tick
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;
	
	void SetTargetWheelAngle(float turnVal);
	void UpdateWheelAngle(float DeltaTime);
	void SetAccelPedalPosition(float pedalVal);
	void SetBrakePedalPosition(float pedalVal);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UFUNCTION()
	float GetVehicleCurrentSpeed() const;
	
	UFUNCTION()
	float GetCurrentGearRatio() const;

private:
	///Calculates and applies suspension forces and positions wheel mesh. 
	///Returns force applied to wheel
	FVector ApplySuspensionForce(TObjectPtr<UWheelSceneComponent> Wheel);
	FVector ApplyLateralForces(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime);
	void CalculateAccelerationForces(TObjectPtr<UWheelSceneComponent> Wheel);
	void UpdateCurrentRPM(float DeltaTime);
	float GetTorqueAtRPM(float RPM) const;
};
