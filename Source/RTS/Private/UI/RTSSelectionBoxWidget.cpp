// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RTSSelectionBoxWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

void URTSSelectionBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectionBox)
	{
		SelectionBoxSlot = Cast<UCanvasPanelSlot>(SelectionBox->Slot);
		SelectionBoxMID = SelectionBox->GetDynamicMaterial();
	}

	SetBoxVisible(false);
}

void URTSSelectionBoxWidget::SetBoxVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void URTSSelectionBoxWidget::SetSelectionBox(const FVector2D& BoxStart, const FVector2D& BoxEnd)
{
	if (!SelectionBoxSlot)
	{
		return;
	}

	// BoxStart/BoxEnd arrive as raw viewport-relative pixel coordinates (from GetMousePosition).
	// UCanvasPanelSlot::SetPosition/SetSize expect the canvas's local (DPI-scaled) space, so
	// divide out the viewport scale (same formula as UWidgetLayoutLibrary::GetMousePositionScaledByDPI).
	// Note: AbsoluteToLocal() is NOT appropriate here since it expects desktop/absolute coordinates
	// (e.g. FSlateApplication::GetCursorPos()), not viewport-relative ones -- using it with a
	// viewport-relative input reintroduces a Y offset in PIE due to the editor toolbar.
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D LocalStart = BoxStart / ViewportScale;
	const FVector2D LocalEnd = BoxEnd / ViewportScale;

	const FVector2D BoxMin(FMath::Min(LocalStart.X, LocalEnd.X), FMath::Min(LocalStart.Y, LocalEnd.Y));
	const FVector2D BoxSize(FMath::Abs(LocalEnd.X - LocalStart.X), FMath::Abs(LocalEnd.Y - LocalStart.Y));

	SelectionBoxSlot->SetPosition(BoxMin);
	SelectionBoxSlot->SetSize(BoxSize);

	if (SelectionBoxMID)
	{
		// Feed the box's on-screen size to the material so it can convert a constant
		// pixel border thickness into per-axis UV thickness, instead of the border
		// scaling (and skewing between X/Y) with the box's own size.
		SelectionBoxMID->SetScalarParameterValue(TEXT("BoxWidthPixels"), FMath::Max(BoxSize.X, 1.0f));
		SelectionBoxMID->SetScalarParameterValue(TEXT("BoxHeightPixels"), FMath::Max(BoxSize.Y, 1.0f));
	}
}
