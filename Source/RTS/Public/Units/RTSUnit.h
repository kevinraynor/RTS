// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSEntityBase.h"
#include "GameFramework/Actor.h"
#include "RTSUnit.generated.h"

class USkeletalMeshComponentBudgeted;
class UCapsuleComponent;

UCLASS()
class RTS_API ARTSUnit : public ARTSEntityBase
{
	GENERATED_BODY()

public:
	ARTSUnit();

	virtual void BeginPlay() override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float Health = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float Energy = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float MovementSpeed = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float TurnRate = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float Damage = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float AttackSpeed = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float AttackRange = 1000.0f;
	
protected:
	// Laid on its side (long axis along forward) so radius/half-height alone can represent
	// anything from a roughly circular infantry footprint to an elongated tank or ship.
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<USkeletalMeshComponentBudgeted> Mesh;
};
