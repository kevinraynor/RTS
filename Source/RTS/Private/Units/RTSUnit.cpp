// Fill out your copyright notice in the Description page of Project Settings.


#include "RTS/Public/Units/RTSUnit.h"

#include "SkeletalMeshComponentBudgeted.h"
#include "Components/CapsuleComponent.h"


ARTSUnit::ARTSUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// Default is roughly circular (radius == half-height); vehicle/ship Blueprints should
	// grow CapsuleHalfHeight relative to CapsuleRadius to get an elongated footprint instead.
	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitCapsuleSize(50.0f, 50.0f);
	CollisionComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // lie flat, long axis along forward
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
	// Buildings/terrain stay hard blocks; other units overlap so our own separation logic
	// (not a hard sweep stop) resolves crowding between them.
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(CollisionComponent);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponentBudgeted>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	// Movement/avoidance collision will live on a separate capsule; the mesh itself
	// only needs to be traceable so click/box selection can hit it.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->bReceivesDecals = false;
}

void ARTSUnit::BeginPlay()
{
	Super::BeginPlay();
	
}