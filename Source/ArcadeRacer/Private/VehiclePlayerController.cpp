// Fill out your copyright notice in the Description page of Project Settings.


#include "VehiclePlayerController.h"

AVehiclePlayerController::AVehiclePlayerController()
{

}

void AVehiclePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//Ensure player is possessing vehicle
	PlayerCarPawn = Cast<AArcadeCarPawn>(GetPawn());
	if (PlayerCarPawn == nullptr) { UE_LOG(LogTemp, Warning, TEXT("No Vehicle pawn found!")); return; }
	
	if (InputComponent == nullptr)
	{
		if (Cast<UEnhancedInputComponent>(InputComponent) == nullptr) UE_LOG(LogTemp, Error, TEXT("UEnhancedInputComponent is nullptr"));
	}
}

void AVehiclePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (TObjectPtr<UEnhancedInputComponent> EInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (TObjectPtr<UEnhancedInputLocalPlayerSubsystem> Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			UE_LOG(LogPlayerController, Display, TEXT("Mapping Context Set"));
			Subsystem->AddMappingContext(InputMappingContext, 0);
		} 
		
		EInputComponent->BindAction(AccelerateAction, ETriggerEvent::Triggered,this, &AVehiclePlayerController::OnAccelerateAction);
		EInputComponent->BindAction(BrakeAction,ETriggerEvent::Triggered,this, &AVehiclePlayerController::OnBrakeAction);
		EInputComponent->BindAction(HandbrakeAction,ETriggerEvent::Triggered,this, &AVehiclePlayerController::OnHandbrakeAction);
		EInputComponent->BindAction(TurnAction,ETriggerEvent::Triggered,this, &AVehiclePlayerController::OnTurnAction);
		
		EInputComponent->BindAction(AccelerateAction, ETriggerEvent::Completed, this, &AVehiclePlayerController::OnAccelerateActionEND);
		EInputComponent->BindAction(BrakeAction,ETriggerEvent::Completed, this, &AVehiclePlayerController::OnBrakeActionEND);
		EInputComponent->BindAction(TurnAction,ETriggerEvent::Completed, this, &AVehiclePlayerController::OnTurnActionEND);
		
		UE_LOG(LogPlayerController, Warning, TEXT("Vehicle Pawn SetupInputComponent Complete"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Vehicle Pawn Setup INCOMPLETE"));
	}
}

void AVehiclePlayerController::Tick(float DeltaTime)
{
	//TODO: Is this needed?
	Super::Tick(DeltaTime);
}

void AVehiclePlayerController::OnTurnAction(const FInputActionValue& Value)
{
	PlayerCarPawn->SetTargetWheelAngle(Value.Get<float>());
}

void AVehiclePlayerController::OnTurnActionEND(const FInputActionValue& Value)
{
	PlayerCarPawn->SetTargetWheelAngle(0.0f);
}

void AVehiclePlayerController::OnAccelerateAction(const FInputActionValue& Value)
{
	PlayerCarPawn->SetAccelPedalPosition(Value.Get<float>());
}

void AVehiclePlayerController::OnAccelerateActionEND(const FInputActionValue& Value)
{
	PlayerCarPawn->SetAccelPedalPosition(0.0f);
}

void AVehiclePlayerController::OnBrakeAction(const FInputActionValue& Value)
{
	PlayerCarPawn->SetBrakePedalPosition(Value.Get<float>());
}

void AVehiclePlayerController::OnBrakeActionEND(const FInputActionValue&)
{
	PlayerCarPawn->SetBrakePedalPosition(0.0f);
}

void AVehiclePlayerController::OnHandbrakeAction(const FInputActionValue& Value)
{
	
}
