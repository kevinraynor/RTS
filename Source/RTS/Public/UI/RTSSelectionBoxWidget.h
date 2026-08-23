// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionBoxWidget.generated.h"

class UBorder;
class UCanvasPanelSlot;
class UMaterialInstanceDynamic;

UCLASS()
class RTS_API URTSSelectionBoxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSelectionBox(const FVector2D& BoxStart, const FVector2D& BoxEnd);
	void SetBoxVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;

	// Bind to a Border named "SelectionBox" inside a CanvasPanel in the widget Blueprint.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SelectionBox;

private:
	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> SelectionBoxSlot;

	// Dynamic instance of the SelectionBox border's material, so its on-screen pixel
	// size can be fed in as a parameter (see SetSelectionBox) and border thickness kept
	// constant in pixels regardless of box size/aspect ratio.
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SelectionBoxMID;
};
