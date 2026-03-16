// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArcadeCarPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "VehiclePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ARCADERACER_API AVehiclePlayerController : public APlayerController
{
	GENERATED_BODY()
	
	AVehiclePlayerController();
	
	protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> TurnAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> AccelerateAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> BrakeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> HandbrakeAction;
	
	//TODO: Implement shunt
	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UInputAction> ShuntL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UInputAction> ShuntR;
	*/
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AArcadeCarPawn> PlayerCarPawn = nullptr;

	///FUNCTIONS
	protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;
	
	void OnTurnAction(const FInputActionValue& Value);
	void OnTurnActionEND(const FInputActionValue& Value);
	void OnAccelerateAction(const FInputActionValue& Value);
	void OnAccelerateActionEND(const FInputActionValue& Value);
	void OnBrakeAction(const FInputActionValue& Value);
	void OnBrakeActionEND(const FInputActionValue&);
	void OnHandbrakeAction(const FInputActionValue& Value);
};
