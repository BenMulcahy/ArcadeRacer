// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WheelSceneComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARCADERACER_API UWheelSceneComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWheelSceneComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelRadius = 15.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionRestDistance = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSuspensionTravel = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionStrength = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionDamping = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite) //TODO: 0-1 range 0 no grip 1 full grip
	float TyreGrip = 0.1f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float TyreRollingResistance = 0.6f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TyreMass = 9.0;
	
	///Rads/S
	float WheelAngularVelocity;
	float WheelRPM;
	
	//TODO: Make setters and getters and private these
	float SuspensionForce = 0.0f;
	FVector WheelAnchorPoint = FVector(0.0f, 0.0f, 0.0f);
	bool bIsGrounded = false;
	
	private:
	FVector WheelMeshTargetLocation;

	public:
	UFUNCTION(BlueprintCallable)
	virtual void SetWheelMeshLocation(const FVector& NewLocation);
	
	UFUNCTION(BlueprintCallable)
	virtual FVector GetWheelMeshLocation();

	UFUNCTION(BlueprintCallable)
	virtual float GetWheelAngularVelocity();
	float GetWheelRPM();
};

		

