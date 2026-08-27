// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSPlayerController.generated.h"

class ARTSBuilding;
class ARTSUnit;
class ARTSCamera;
class UInputMappingContext;
class UInputAction;
class URTSSelectionBoxWidget;

/**
 *
 */
UCLASS()
class RTS_API ARTSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARTSPlayerController();
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	void StartDragSelection();

	UFUNCTION(BlueprintCallable)
	void EndDragSelection();

	UFUNCTION(BlueprintCallable)
	void ClearSelection();

	UFUNCTION(BlueprintCallable)
	void Action();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|Input")
	TObjectPtr<UInputAction> SelectAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|Input")
	TObjectPtr<UInputAction> ActionAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|Input")
	TObjectPtr<UInputAction> PanCameraAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|UI")
	TSubclassOf<URTSSelectionBoxWidget> SelectionBoxWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="RTS|Camera")
	TSubclassOf<ARTSCamera> CameraClass;
	
	// Fraction of the shorter viewport dimension used as the edge-scroll margin.
	UPROPERTY(EditDefaultsOnly, Category="RTS|Camera")
	float EdgeMarginFraction = 0.125f;
	
	UPROPERTY()
	TObjectPtr<ARTSCamera> CameraPawn;

private:
	void UpdateDragSelection();
	void FindUnitsInSelectionBox(const FVector2D& BoxStart, const FVector2D& BoxEnd);

private:
	// Below this drag distance (px) a select is treated as a click on a point rather than a box.
	static constexpr float ClickDragThreshold = 5.0f;

	bool bIsDragSelecting = false;
	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;
	
	UPROPERTY()
	TObjectPtr<URTSSelectionBoxWidget> SelectionBoxWidget;

	TWeakObjectPtr<ARTSBuilding> SelectedBuilding;
	TArray<TWeakObjectPtr<ARTSUnit>> SelectedUnits;
};
