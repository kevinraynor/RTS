// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "RTSEntityBase.generated.h"

UCLASS()
class RTS_API ARTSEntityBase : public AActor
{
	GENERATED_BODY()

public:
	ARTSEntityBase();
	
	
public:
	// ------------------------------------------------------------------------
	// BASE
	// ------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	FGameplayTag UniqueID; 
	
	// ------------------------------------------------------------------------
	// BUY/CREATE
	// ------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float Cost = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	float ConstructTime = 10.0f;

	// ------------------------------------------------------------------------
	// Display
	// ------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	TObjectPtr<UTexture2D> Icon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	FName Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	FText Description;
};
