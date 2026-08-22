// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionBoxWidget.generated.h"

class UBorder;
class UCanvasPanelSlot;

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
};
