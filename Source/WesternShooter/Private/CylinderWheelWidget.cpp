#include "CylinderWheelWidget.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "WesternShooterCharacter.h"

URevolverCylinderComponent* UCylinderWheelWidget::GetCylinder() const
{
	// GetOwningPlayerPawn returns the Pawn controlled by the player this widget belongs to.
	// It is looked up each time, because the Pawn is replaced when the player respawns.
	const APawn* Pawn = GetOwningPlayerPawn();

	// FindComponentByClass returns the first component of that class on the Actor, or null.
	return Pawn ? Pawn->FindComponentByClass<URevolverCylinderComponent>() : nullptr;
}

float UCylinderWheelWidget::GetCockProgress() const
{
	const URevolverCylinderComponent* Cylinder = GetCylinder();
	if (!Cylinder || !Cylinder->IsCocking() || !GetWorld())
	{
		return 0.0f;
	}

	// The widget measures from the moment it first saw IsCocking become true.
	// On a client this starts slightly after the server's timer, by the network delay.
	const float Elapsed = GetWorld()->GetTimeSeconds() - CockObservedStartTime;
	const float Required = FMath::Max(Cylinder->GetCurrentChamberCockTime(), 0.01f);

	return FMath::Clamp(Elapsed / Required, 0.0f, 1.0f);
}

float UCylinderWheelWidget::GetMouseAngle(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const
{
	// Convert the mouse position from screen coordinates to this widget's own coordinates.
	const FVector2D LocalPosition = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D Center = FVector2D(Geometry.GetLocalSize()) * 0.5;
	const FVector2D Offset = LocalPosition - Center;

	// Atan2 gives the angle of the offset vector. In UI coordinates Y points down,
	// so a larger angle means clockwise, which matches the direction of positive widget rotation.
	return FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
}

int32 UCylinderWheelWidget::AngleToIndex(float Angle) const
{
	// With the wheel at angle 0, chamber 0 is at the top, under the hammer.
	// Chamber k is drawn k * 60 degrees clockwise from the top.
	// To bring chamber k to the top, the wheel turns k * 60 degrees counterclockwise, so its angle is -k * 60.
	const int32 Count = URevolverCylinderComponent::ChamberCount;
	const int32 Steps = FMath::RoundToInt(-Angle / ChamberAngle);

	// Wraps negative and large values into the range 0 to Count - 1.
	return ((Steps % Count) + Count) % Count;
}

FReply UCylinderWheelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	URevolverCylinderComponent* Cylinder = GetCylinder();

	// The wheel cannot be grabbed during a reload or while cocking. The server would reject the spin.
	if (!Cylinder
		|| !bWheelOpen
		|| InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton
		|| Cylinder->IsReloading()
		|| Cylinder->IsCocking())
	{
		return FReply::Unhandled();
	}

	// Tell the server once, at the first grab. Grabbing an already spinning wheel does not send it again.
	if (!bLocalSpinActive)
	{
		Cylinder->BeginSpin();
		bLocalSpinActive = true;
	}

	// Grabbing stops the wheel. This is what lets the player halt a spin by holding it.
	bGrabbed = true;
	bSnapping = false;
	AngularVelocity = 0.0f;
	PendingDragDelta = 0.0f;
	LastMouseAngle = GetMouseAngle(InGeometry, InMouseEvent);

	// Capturing the mouse makes this widget keep receiving mouse events while the button is held,
	// even when the cursor leaves the widget's area.
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCylinderWheelWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bGrabbed)
	{
		return FReply::Unhandled();
	}

	const float MouseAngle = GetMouseAngle(InGeometry, InMouseEvent);

	// FindDeltaAngleDegrees returns the shortest signed difference between two angles.
	// It handles the jump between 180 and -180 degrees.
	const float Delta = FMath::FindDeltaAngleDegrees(LastMouseAngle, MouseAngle);

	// The wheel turns by the same amount the cursor moved around the center.
	WheelAngle += Delta;
	PendingDragDelta += Delta;
	LastMouseAngle = MouseAngle;

	return FReply::Handled();
}

FReply UCylinderWheelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	if (!bGrabbed)
	{
		return FReply::Handled().ReleaseMouseCapture();
	}

	bGrabbed = false;

	return FReply::Handled().ReleaseMouseCapture();
}

void UCylinderWheelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Moves the wheel between its closed and open positions.
	const float TargetAlignmentY = bWheelOpen ? OpenAlignmentY : ClosedAlignmentY;
	if (!FMath::IsNearlyEqual(CurrentAlignmentY, TargetAlignmentY, 0.0001f))
	{
		CurrentAlignmentY = FMath::FInterpTo(CurrentAlignmentY, TargetAlignmentY, InDeltaTime, SlideInterpSpeed);

		// Changes which point of the widget sits on its anchor. The anchor stays at the bottom center of the screen.
		SetAlignmentInViewport(FVector2D(0.5f, CurrentAlignmentY));
	}

	if (bWheelOpen)
	{
		const AWesternShooterCharacter* Character = Cast<AWesternShooterCharacter>(GetOwningPlayerPawn());
		if (Character && Character->IsDead())
		{
			CloseWheel();
		}
	}

	URevolverCylinderComponent* Cylinder = GetCylinder();
	if (!Cylinder || !GetWorld() || InDeltaTime <= 0.0f)
	{
		return;
	}

	BindToCylinder(Cylinder);

	const float Now = GetWorld()->GetTimeSeconds();

	// Tell the Blueprint to redraw the bullet icons only when the contents changed.
	const TArray<EBulletType>& CurrentChambers = Cylinder->GetChambers();
	if (CurrentChambers != CachedChambers)
	{
		CachedChambers = CurrentChambers;
		UpdateBulletIcons();
		OnChambersChanged(CachedChambers);
	}

	// Record the time at which cocking was first seen, for GetCockProgress.
	const bool bCockingNow = Cylinder->IsCocking();
	if (bCockingNow && !bWasCocking)
	{
		CockObservedStartTime = Now;
	}
	bWasCocking = bCockingNow;

	if (CockBar)
	{
		CockBar->SetPercent(GetCockProgress());
	}

	if (bGrabbed)
	{
		// Speed of the drag during this frame, in degrees per second.
		const float InstantVelocity = PendingDragDelta / InDeltaTime;
		PendingDragDelta = 0.0f;

		// Blend with the previous value so one uneven frame does not decide the release speed.
		// If the mouse is held still, the blended speed falls to zero within a few frames.
		AngularVelocity = FMath::Lerp(AngularVelocity, InstantVelocity, 0.5f);
		AngularVelocity = FMath::Clamp(AngularVelocity, -MaxSpinSpeed, MaxSpinSpeed);
	}
	else if (bLocalSpinActive)
	{
		if (!bSnapping)
		{
			// Free spin: turn by the current speed, then reduce the speed by friction.
			WheelAngle += AngularVelocity * InDeltaTime;
			AngularVelocity *= FMath::Exp(-Damping * InDeltaTime);

			if (FMath::Abs(AngularVelocity) < SnapSpeedThreshold)
			{
				// Slow enough: pick the nearest chamber position and move onto it.
				bSnapping = true;
				SnapTargetAngle = FMath::RoundToFloat(WheelAngle / ChamberAngle) * ChamberAngle;
			}
		}
		else
		{
			// FInterpTo moves a value toward a target, quickly at first and slower as it gets close.
			WheelAngle = FMath::FInterpTo(WheelAngle, SnapTargetAngle, InDeltaTime, SnapInterpSpeed);

			if (FMath::IsNearlyEqual(WheelAngle, SnapTargetAngle, 0.5f))
			{
				WheelAngle = SnapTargetAngle;
				FinishSpin();
			}
		}
	}
	else
	{
		// Not spinning: the wheel shows the server's active chamber.
		// This is what turns the wheel after a shot, a cycle, or a reload step.
		if (PendingIndex != INDEX_NONE)
		{
			// Wait for the server to confirm the index sent at the end of the last spin, or give up after the timeout.
			if (Cylinder->GetCurrentIndex() == PendingIndex || Now >= PendingIndexExpireTime)
			{
				PendingIndex = INDEX_NONE;
			}
		}

		if (PendingIndex == INDEX_NONE)
		{
			const float TargetAngle = -Cylinder->GetCurrentIndex() * ChamberAngle;

			// Turn through the shorter direction toward the target.
			const float Delta = FMath::FindDeltaAngleDegrees(WheelAngle, TargetAngle);
			WheelAngle += Delta * FMath::Clamp(InDeltaTime * FollowInterpSpeed, 0.0f, 1.0f);
		}
	}

	// Apply the rotation to everything inside WheelRoot.
	if (WheelRoot)
	{
		WheelRoot->SetRenderTransformAngle(WheelAngle);
	}
}

void UCylinderWheelWidget::ForceSnap()
{
	if (!bLocalSpinActive)
	{
		return;
	}

	// Jump straight to the nearest chamber position, skipping the slow-down.
	WheelAngle = FMath::RoundToFloat(WheelAngle / ChamberAngle) * ChamberAngle;
	FinishSpin();
}

void UCylinderWheelWidget::FinishSpin()
{
	const int32 LandedIndex = AngleToIndex(WheelAngle);

	if (URevolverCylinderComponent* Cylinder = GetCylinder())
	{
		Cylinder->EndSpin(LandedIndex);
	}

	bLocalSpinActive = false;
	bSnapping = false;
	bGrabbed = false;
	AngularVelocity = 0.0f;

	// Keep the angle in the range -180 to 180, so it does not grow without limit over many spins.
	WheelAngle = FMath::UnwindDegrees(WheelAngle);

	// Hold the wheel here until the server reports the same index, for at most half a second.
	PendingIndex = LandedIndex;
	PendingIndexExpireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.5f : 0.0f;
}

void UCylinderWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Hides all icons at the start, before any chamber data has arrived.
	UpdateBulletIcons();
}

void UCylinderWheelWidget::UpdateBulletIcons()
{
	const TArray<UImage*> Images = { Bullet0, Bullet1, Bullet2, Bullet3, Bullet4, Bullet5 };

	for (int32 Index = 0; Index < Images.Num(); ++Index)
	{
		UImage* Image = Images[Index];
		if (!Image)
		{
			continue;
		}

		const EBulletType Type = CachedChambers.IsValidIndex(Index) ? CachedChambers[Index] : EBulletType::Empty;
		const TObjectPtr<UTexture2D>* Texture = BulletTextures.Find(Type);

		if (Texture && *Texture)
		{
			// The second argument false keeps the image size set in the designer.
			Image->SetBrushFromTexture(*Texture, false);

			// HitTestInvisible: drawn, but mouse clicks pass through it to the cylinder image underneath.
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UCylinderWheelWidget::BindToCylinder(URevolverCylinderComponent* Cylinder)
{
	if (BoundCylinder.Get() == Cylinder)
	{
		return;
	}

	if (URevolverCylinderComponent* OldCylinder = BoundCylinder.Get())
	{
		OldCylinder->OnLocalCockRequested.Remove(CockRequestedHandle);
	}

	BoundCylinder = Cylinder;
	CockRequestedHandle.Reset();

	bLocalSpinActive = false;
	bSnapping = false;
	bGrabbed = false;
	AngularVelocity = 0.0f;
	PendingIndex = INDEX_NONE;
	CachedChambers.Reset();

	if (Cylinder)
	{
		CockRequestedHandle = Cylinder->OnLocalCockRequested.AddUObject(this, &UCylinderWheelWidget::HandleCockRequested);
	}
}

void UCylinderWheelWidget::HandleCockRequested()
{
	ForceSnap();
}

void UCylinderWheelWidget::NativeDestruct()
{
	BindToCylinder(nullptr);
	Super::NativeDestruct();
}

void UCylinderWheelWidget::OpenWheel()
{
	if (bWheelOpen)
	{
		return;
	}

	// The wheel cannot open while the hammer is being pulled back.
	const URevolverCylinderComponent* Cylinder = GetCylinder();
	if (Cylinder && Cylinder->IsCocking())
	{
		return;
	}

	// GetOwningPlayer returns the PlayerController this widget was created for.
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	bWheelOpen = true;

	// Game and UI mode: mouse clicks can reach widgets, and keyboard input still reaches the game, so walking works.
	FInputModeGameAndUI InputMode;

	// Keeps the cursor visible while a mouse button is held, which is when the wheel is dragged.
	InputMode.SetHideCursorDuringCapture(false);

	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);

	// Mouse movement no longer turns the camera. This is a counter in the engine:
	// each true must be matched by one false, which bWheelOpen guarantees.
	PlayerController->SetIgnoreLookInput(true);
}

void UCylinderWheelWidget::CloseWheel()
{
	if (!bWheelOpen)
	{
		return;
	}

	bWheelOpen = false;

	// If the wheel was being dragged, let go of it. It keeps the speed it had and spins freely.
	bGrabbed = false;

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	// Game only mode: the viewport takes the mouse back, which also ends this widget's mouse capture.
	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->SetShowMouseCursor(false);
	PlayerController->SetIgnoreLookInput(false);
}