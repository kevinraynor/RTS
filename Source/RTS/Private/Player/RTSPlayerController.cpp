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

ARTSPlayerController::ARTSPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

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

		CameraPawn = GetWorld()->SpawnActor<ARTSCamera>(CameraClass, SpawnTransform);
		if (CameraPawn)
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
		if (ActionAction)
		{
			EnhancedInputComponent->BindAction(ActionAction, ETriggerEvent::Triggered, this, &ARTSPlayerController::Action);
		}
		if (PanCameraAction)
		{
			EnhancedInputComponent->BindAction(PanCameraAction, ETriggerEvent::Started, this, &ARTSPlayerController::StartDragSelection);
			EnhancedInputComponent->BindAction(PanCameraAction, ETriggerEvent::Triggered, this, &ARTSPlayerController::UpdateDragSelection);
			EnhancedInputComponent->BindAction(PanCameraAction, ETriggerEvent::Completed, this, &ARTSPlayerController::EndDragSelection);
		}
	}
}

void ARTSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Verify if player's cursor location is at the edge of the window, if so; move the camera.
	FVector2D CurrentScreenPosition;
	if (!GetMousePosition(CurrentScreenPosition.X, CurrentScreenPosition.Y))
	{
		// Cursor isn't over the viewport (e.g. window unfocused) - nothing to do.
		return;
	}

	int32 ViewportWidth, ViewportHeight;
	GetViewportSize(ViewportWidth, ViewportHeight);
	FVector2D ViewportSize(ViewportWidth, ViewportHeight);

	// [0.0 - 1.0] cursor position relative to viewport size, used only for direction below.
	FVector2D NormalizedMousePosition = CurrentScreenPosition / ViewportSize;

	// Margin in pixels, same thickness on both axes, scaled off the shorter viewport
	// dimension so it stays proportionate across resolutions and aspect ratios.
	const float EdgeMarginPixels = EdgeMarginFraction * FMath::Min(ViewportWidth, ViewportHeight);
	const bool bNearEdgeX = CurrentScreenPosition.X < EdgeMarginPixels || CurrentScreenPosition.X > ViewportWidth - EdgeMarginPixels;
	const bool bNearEdgeY = CurrentScreenPosition.Y < EdgeMarginPixels || CurrentScreenPosition.Y > ViewportHeight - EdgeMarginPixels;

	if (bNearEdgeX || bNearEdgeY)
	{
		// Cursor is close to the edge, start moving.
		FVector2D MoveDirection = NormalizedMousePosition * 2.0 - 1.0;
		MoveDirection.Normalize();

		if (IsValid(CameraPawn))
		{
			CameraPawn->Move(DeltaSeconds, MoveDirection);
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

void ARTSPlayerController::Action()
{
	FVector2D CommandPosition;
	GetMousePosition(CommandPosition.X, CommandPosition.Y);
	
	// TODO: project screen pos to world, use trace to find object.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	FHitResult HitResult;
	if (GetHitResultUnderCursorForObjects(ObjectTypes, false, HitResult))
	{
		// A game entity has been hit, evaluate what should happen.
		AActor* Actor = HitResult.GetActor();
		if (ARTSUnit* Unit = Cast<ARTSUnit>(Actor))
		{
			// TODO: Attack via combat component.
		}
		else if (ARTSBuilding* Building = Cast<ARTSBuilding>(Actor))
		{
			// TODO: Attack, or enter, or do other interesting stuff based on building type and its component(s).
		}
	}
	else 
	{
		// Nothing of a gameplay entity was hit, must be terrain/buildings/decor. 
		// If units are selected, this becomes a "Move" action. If a building is selected
		// it becomes a "Place Spawn/Flag Location" action.
		// TODO: Move.
		if (SelectedUnits.Num() > 0)
		{
			// TODO: Move via movement system.
		}
		else if (SelectedBuilding.IsValid())
		{
			// TODO: Attack, or enter, or do other interesting stuff based on building type and its component(s).
		}
	}
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
		// TODO: might it not be more efficient to use box. trace here? If not, look into spatial grid tree of unit placement to query.
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
