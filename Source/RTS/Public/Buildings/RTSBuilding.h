// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSEntityBase.h"
#include "GameFramework/Actor.h"
#include "RTSBuilding.generated.h"

class ARTSUnit;
class URTSUpgrade;
class USkeletalMeshComponentBudgeted;
class UBoxComponent;

UCLASS()
class RTS_API ARTSBuilding : public ARTSEntityBase
{
	GENERATED_BODY()

public:
	ARTSBuilding();

	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	virtual void Purchase(int32 Index);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RTS")
	TArray<TSubclassOf<ARTSEntityBase>> Purchasables;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="RTS")
	FVector SpawnLocation;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS")
	TObjectPtr<USkeletalMeshComponentBudgeted> Mesh;
};
