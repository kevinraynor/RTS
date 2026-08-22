// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;

// Lightweight top-down view pawn: no movement/gameplay of its own, just a spring arm +
// camera at a fixed pitch. Possessed explicitly by ARTSPlayerController rather than via
// GameMode's DefaultPawnClass flow, so it never fights the RTS's own selection/units logic.
UCLASS()
class RTS_API ARTSCamera : public APawn
{
	GENERATED_BODY()

public:
	ARTSCamera();

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<UCameraComponent> Camera;
};
