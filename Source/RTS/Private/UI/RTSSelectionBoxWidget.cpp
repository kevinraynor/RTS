// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RTSSelectionBoxWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void URTSSelectionBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectionBox)
	{
		SelectionBoxSlot = Cast<UCanvasPanelSlot>(SelectionBox->Slot);
	}

	SetBoxVisible(false);
}

void URTSSelectionBoxWidget::SetSelectionBox(const FVector2D& BoxStart, const FVector2D& BoxEnd)
{
	if (!SelectionBoxSlot)
	{
		return;
	}

	const FVector2D BoxMin(FMath::Min(BoxStart.X, BoxEnd.X), FMath::Min(BoxStart.Y, BoxEnd.Y));
	const FVector2D BoxSize(FMath::Abs(BoxEnd.X - BoxStart.X), FMath::Abs(BoxEnd.Y - BoxStart.Y));

	SelectionBoxSlot->SetPosition(BoxMin);
	SelectionBoxSlot->SetSize(BoxSize);
}

void URTSSelectionBoxWidget::SetBoxVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
