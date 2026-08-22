// Fill out your copyright notice in the Description page of Project Settings.


#include "RTS/Public/Player/RTSPlayerController.h"

#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Buildings/RTSBuilding.h"
#include "GameFramework/PlayerStart.h"
#include "Player/RTSCamera.h"
#include "UI/RTSSelectionBoxWidget.h"
#include "Units/RTSUnit.h"

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (SelectionBoxWidgetClass)
	{
		SelectionBoxWidget = CreateWidget<URTSSelectionBoxWidget>(this, SelectionBoxWidgetClass);
		if (SelectionBoxWidget)
		{
			SelectionBoxWidget->AddToViewport();
		}
	}

	if (HasAuthority() && CameraClass)
	{
		FTransform SpawnTransform = FTransform::Identity;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			SpawnTransform = It->GetActorTransform();
			break;
		}

		if (ARTSCamera* CameraPawn = GetWorld()->SpawnActor<ARTSCamera>(CameraClass, SpawnTransform))
		{
			Possess(CameraPawn);
		}
	}
}

void ARTSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SelectAction)
		{
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ARTSPlayerController::StartDragSelection);
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Triggered, this, &ARTSPlayerController::UpdateDragSelection);
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ARTSPlayerController::EndDragSelection);
		}
	}
}

void ARTSPlayerController::StartDragSelection()
{
	bIsDragSelecting = true;
	GetMousePosition(DragStartScreenPosition.X, DragStartScreenPosition.Y);

	if (SelectionBoxWidget)
	{
		SelectionBoxWidget->SetSelectionBox(DragStartScreenPosition, DragStartScreenPosition);
		SelectionBoxWidget->SetBoxVisible(true);
	}
}

void ARTSPlayerController::UpdateDragSelection()
{
	if (!bIsDragSelecting || !SelectionBoxWidget)
	{
		return;
	}

	FVector2D CurrentScreenPosition;
	GetMousePosition(CurrentScreenPosition.X, CurrentScreenPosition.Y);
	SelectionBoxWidget->SetSelectionBox(DragStartScreenPosition, CurrentScreenPosition);
}

void ARTSPlayerController::EndDragSelection()
{
	if (!bIsDragSelecting)
	{
		return;
	}
	bIsDragSelecting = false;

	if (SelectionBoxWidget)
	{
		SelectionBoxWidget->SetBoxVisible(false);
	}

	FVector2D DragEndScreenPosition;
	GetMousePosition(DragEndScreenPosition.X, DragEndScreenPosition.Y);

	ClearSelection();
	FindUnitsInSelectionBox(DragStartScreenPosition, DragEndScreenPosition);
}

void ARTSPlayerController::ClearSelection()
{
	SelectedBuilding.Reset();
	SelectedUnits.Reset();
}

void ARTSPlayerController::Command()
{
}

void ARTSPlayerController::FindUnitsInSelectionBox(const FVector2D& BoxStart, const FVector2D& BoxEnd)
{	
	const FVector2D BoxMin(FMath::Min(BoxStart.X, BoxEnd.X), FMath::Min(BoxStart.Y, BoxEnd.Y));
	const FVector2D BoxMax(FMath::Max(BoxStart.X, BoxEnd.X), FMath::Max(BoxStart.Y, BoxEnd.Y));
	const bool bIsClick = (BoxMax - BoxMin).SizeSquared() < FMath::Square(ClickDragThreshold);

	TWeakObjectPtr<ARTSBuilding> NewSelectedBuilding;
	TArray<TWeakObjectPtr<ARTSUnit>> NewSelectedUnits;
	for (TActorIterator<ARTSUnit> It(GetWorld()); It; ++It)
	{
		ARTSUnit* Unit = *It;
		FVector2D ScreenPosition;
		if (!ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPosition))
		{
			continue;
		}

		const bool bInBox = bIsClick
			? FVector2D::Distance(ScreenPosition, BoxMin) < ClickDragThreshold * 2.0f
			: (ScreenPosition.X >= BoxMin.X && ScreenPosition.X <= BoxMax.X &&
			   ScreenPosition.Y >= BoxMin.Y && ScreenPosition.Y <= BoxMax.Y);

		if (bInBox)
		{
			NewSelectedUnits.Add(Unit);
		}
	}

	// If no units were selected, only then do we check if we also triggered a building.
	if (NewSelectedUnits.Num() == 0)
	{
		for (TActorIterator<ARTSBuilding> It(GetWorld()); It; ++It)
		{
			ARTSBuilding* Building = *It;
			FVector2D ScreenPosition;
			if (!ProjectWorldLocationToScreen(Building->GetActorLocation(), ScreenPosition))
			{
				continue;
			}

			const bool bInBox = bIsClick
				? FVector2D::Distance(ScreenPosition, BoxMin) < ClickDragThreshold * 2.0f
				: (ScreenPosition.X >= BoxMin.X && ScreenPosition.X <= BoxMax.X &&
				   ScreenPosition.Y >= BoxMin.Y && ScreenPosition.Y <= BoxMax.Y);

			if (bInBox)
			{
				NewSelectedBuilding = Building;
				break;
			}
		}
	}
	
	// If we actually selected 1 or more units, or a building, clear out currently selected
	// entities and replace with current selection.
	ClearSelection();
	if (NewSelectedUnits.Num() > 0)
	{
		SelectedUnits = NewSelectedUnits;
	}
	else
	{
		SelectedBuilding = NewSelectedBuilding;
	}
}
