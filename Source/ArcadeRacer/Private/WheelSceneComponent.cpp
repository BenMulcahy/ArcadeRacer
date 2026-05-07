// Fill out your copyright notice in the Description page of Project Settings.


#include "WheelSceneComponent.h"

// Sets default values for this component's properties
UWheelSceneComponent::UWheelSceneComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	/*
	WheelMesh->CreateDefaultSubobject<UStaticMeshComponent>("Wheel Mesh");
	WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	*/
}

void UWheelSceneComponent::SetWheelMeshLocation(const FVector& NewLocation)
{
	WheelMeshTargetLocation = NewLocation;
}

FVector UWheelSceneComponent::GetWheelMeshLocation()
{
	return WheelMeshTargetLocation;
}

float UWheelSceneComponent::GetAngularVelocity()
{
	return (WheelRPM * 2 * UE_PI) / 60;
}

void UWheelSceneComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//if(WheelMesh != nullptr) WheelMesh->SetRelativeLocation(WheelMeshTargetLocation);
}

