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

float UWheelSceneComponent::GetWheelAngularVelocity()
{
	return WheelAngularVelocity;
}

