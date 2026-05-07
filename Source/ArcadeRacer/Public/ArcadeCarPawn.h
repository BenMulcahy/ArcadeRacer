// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WheelSceneComponent.h"
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

USTRUCT(BlueprintType)
struct FGear
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float GearRatio = 2.5;

	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	int32 GearUpRPM = 5800;

	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	int32 GearDownRPM = 0;
};

//VEHICLE DATA
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
	TObjectPtr<UCurveFloat> TorqueCurve;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxTorque = 180.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MaxRPM = 7800;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 IdleRPM = 1200;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float EngineBreaking = 0.2f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ReturnToIdleStrength = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Vehicle Data | Engine")
	float TransmissionCouplingScalar = 1.0f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Vehicle Data | Gearing")
	TMap<FString, FGear> Gears = {
		{"reverse", FGear(-2.9,7800,1200)},
		{"neutral", FGear(0.0f, 0,0)},
		{"1", FGear(2.5f,7800,1200)},
		{"2", FGear(1.6f,7800,1200)},
		{"3", FGear(1.1f,7800,1200)},
		{"4", FGear(0.81f,7800,1200)},
		{"5", FGear(0.6f,7800,1200)},
	};
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "VehicleData | Gearing")
	float GearDifferential = 4.0f;
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
	TObjectPtr<UWheelSceneComponent> FL_Wheel;
	
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
	
	//0-1 value of where accel 'pedal' is
	float _Throttle = 0.0f;
	//where brake 'pedal' is
	float _BrakePedalPosition = 0.0f;
	
	float _CurrentEngineRPM = 0.0f;
	float _CurrentEngineTorque = 0.0f;
	FString _CurrentGear = "neutral";
	
	float _AverageDrivenWheelsAngularVelocity;
	
//Functions
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//Async Tick
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;
	void ApplyLongitudinalForces(TObjectPtr<UWheelSceneComponent> Wheel);

	void SetTargetWheelAngle(float turnVal);
	void UpdateWheelAngle(float DeltaTime);
	float GetTorqueAtWheels() const;
	void SetAccelPedalPosition(float pedalVal);
	void SetBrakePedalPosition(float pedalVal);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UFUNCTION()
	float GetVehicleCurrentSpeed() const;

	UFUNCTION(BlueprintCallable)
	void SetCurrentGear(FString gearName);
	
	UFUNCTION()
	float GetCurrentGearRatio() const;
	float GetExpectedRPMatWheels() const;
	static float GetRPMatWheelActual(TObjectPtr<UWheelSceneComponent> Wheel);

private:

	///Calculates and applies suspension forces and positions wheel mesh. 
	///Returns force applied to wheel
	void ApplySuspensionForce(TObjectPtr<UWheelSceneComponent> Wheel) const;
	void ApplyLateralForces(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime) const;
	void CalculateEngineRPM(float DeltaTime);
	void CalculateWheelRPM(TObjectPtr<UWheelSceneComponent> Wheel, float DeltaTime);
	float GetSlipRatio(TObjectPtr<UWheelSceneComponent> Wheel) const;
	//void TryRotateWheels(float DeltaTime);
};
