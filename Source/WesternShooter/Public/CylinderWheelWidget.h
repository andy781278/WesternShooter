#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RevolverCylinderComponent.h"
#include "CylinderWheelWidget.generated.h"

class UWidget;

class UImage;
class UProgressBar;
class UTexture2D;

// The C++ side of the wheel. It holds the rotation math and talks to the cylinder component.
// A Widget Blueprint that uses this class as its parent provides the layout and the art.
UCLASS()
class WESTERNSHOOTER_API UCylinderWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	TMap<EBulletType, TObjectPtr<UTexture2D>> BulletTextures;

	// How quickly a spinning wheel slows down. Each second, the speed is multiplied by e^(-Damping).
	// Higher values stop the wheel sooner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float Damping = 1.5f;

	// When the speed drops below this many degrees per second, the wheel snaps to the nearest chamber.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float SnapSpeedThreshold = 90.0f;

	// How fast the wheel moves to the chamber it is snapping to. Higher is faster.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float SnapInterpSpeed = 15.0f;

	// How fast the wheel turns to match the server's active chamber after a shot, cycle, or reload step.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float FollowInterpSpeed = 15.0f;

	// Upper limit on spin speed, in degrees per second. 2880 is 8 full turns per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float MaxSpinSpeed = 2880.0f;

	// Current rotation of the wheel in degrees. Positive is clockwise.
	UFUNCTION(BlueprintPure, Category = "Wheel")
	float GetWheelAngle() const { return WheelAngle; }

	// 0 when not cocking, rising to 1 when the hammer is ready. For a progress bar.
	UFUNCTION(BlueprintPure, Category = "Wheel")
	float GetCockProgress() const;

	// True while this player's wheel is being dragged or is still turning.
	UFUNCTION(BlueprintPure, Category = "Wheel")
	bool IsWheelSpinning() const { return bLocalSpinActive; }

	// Ends a spin immediately on the nearest chamber. Used when firing or closing the wheel during a spin.
	UFUNCTION(BlueprintCallable, Category = "Wheel")
	void ForceSnap();

	// The cylinder component on the character this player controls, or null if there is none.
	UFUNCTION(BlueprintPure, Category = "Wheel")
	URevolverCylinderComponent* GetCylinder() const;

	UFUNCTION(BlueprintCallable, Category = "Wheel")
	void OpenWheel();

	UFUNCTION(BlueprintCallable, Category = "Wheel")
	void CloseWheel();

	UFUNCTION(BlueprintPure, Category = "Wheel")
	bool IsWheelOpen() const { return bWheelOpen; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float ClosedAlignmentY = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float OpenAlignmentY = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float SlideInterpSpeed = 12.0f;

protected:
	// BindWidget connects this variable to a widget in the Widget Blueprint that has the same name.
	// The Blueprint must contain a widget named WheelRoot, or it will not compile.
	// Everything that should rotate (the cylinder image and the bullet icons) goes inside WheelRoot.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> WheelRoot;

	// BlueprintImplementableEvent: declared in C++ with no C++ body. The Widget Blueprint provides the body.
	// C++ calls it when the chamber contents change, and the Blueprint sets the bullet icons.
	UFUNCTION(BlueprintImplementableEvent, Category = "Wheel")
	void OnChambersChanged(const TArray<EBulletType>& NewChambers);

	// The "Native" functions are the C++ versions of a widget's events. NativeTick runs every frame.
	// Using Tick is appropriate here, because the wheel is animated continuously.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// FGeometry describes the widget's position and size on screen. FPointerEvent describes the mouse event.
	// FReply tells the UI system whether this widget handled the event.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Bullet5;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> CockBar;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// Degrees between neighboring chambers: 360 / 6 = 60.
	static constexpr float ChamberAngle = 360.0f / URevolverCylinderComponent::ChamberCount;

	float WheelAngle = 0.0f;

	// Degrees per second. Positive is clockwise.
	float AngularVelocity = 0.0f;

	// True while the mouse button is held on the wheel.
	bool bGrabbed = false;

	// True from the first grab until the wheel has settled and the result was sent to the server.
	bool bLocalSpinActive = false;

	// True during the final movement onto a chamber.
	bool bSnapping = false;

	float SnapTargetAngle = 0.0f;

	// Angle from the wheel's center to the mouse at the previous mouse event.
	float LastMouseAngle = 0.0f;

	// Rotation applied by mouse movement since the last frame. Used to measure drag speed.
	float PendingDragDelta = 0.0f;

	// After a spin ends, the server's index takes a moment to arrive.
	// Until it matches this value, the wheel does not follow the server, so it does not turn back briefly.
	// INDEX_NONE is Unreal's constant for "no index" (-1).
	int32 PendingIndex = INDEX_NONE;
	float PendingIndexExpireTime = 0.0f;

	// Copy of the chambers from the previous frame, to detect changes.
	TArray<EBulletType> CachedChambers;

	bool bWasCocking = false;
	float CockObservedStartTime = 0.0f;

	float GetMouseAngle(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const;
	int32 AngleToIndex(float Angle) const;
	void FinishSpin();

	void UpdateBulletIcons();

	TWeakObjectPtr<URevolverCylinderComponent> BoundCylinder;
	FDelegateHandle CockRequestedHandle;
	void BindToCylinder(URevolverCylinderComponent* Cylinder);
	void HandleCockRequested();

	bool bWheelOpen = false;
	float CurrentAlignmentY = 0.5f;
};