// Fill out your copyright notice in the Description page of Project Settings.


#include "RTS/Public/Buildings/RTSBuilding.h"

#include "SkeletalMeshComponentBudgeted.h"
#include "Components/BoxComponent.h"

ARTSBuilding::ARTSBuilding()
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAll"));
	SetRootComponent(CollisionComponent);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponentBudgeted>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->bReceivesDecals = false;
}

void ARTSBuilding::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARTSBuilding::Purchase(int32 Index)
{
}

