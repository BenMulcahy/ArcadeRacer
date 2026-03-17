// Fill out your copyright notice in the Description page of Project Settings.


#include "WheelSceneComponent.h"

// Sets default values for this component's properties
UWheelSceneComponent::UWheelSceneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWheelSceneComponent::SetWheelMeshLocation(const FVector& NewLocation)
{
	WheelMeshTargetLocation = NewLocation;
}

FVector UWheelSceneComponent::GetWheelMeshLocation()
{
	return WheelMeshTargetLocation;
}


//TODO: Rework -> This aint greate cus RPM will be wrong if angular Velocity isnt updated
float UWheelSceneComponent::GetWheelAngularVelocity()
{
	return WheelAngularVelocity;
}

float UWheelSceneComponent::GetWheelRPM()
{
	return  WheelAngularVelocity * (60 / (2 * UE_PI));
}

