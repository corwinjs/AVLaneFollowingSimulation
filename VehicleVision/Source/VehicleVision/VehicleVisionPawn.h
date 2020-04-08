#pragma once

#include "CoreMinimal.h"
#include "WheeledVehicle.h"
#include "TCPClientSocket.h"
#include "VehicleVisionPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UTextRenderComponent;
class UInputComponent;
class USceneCaptureComponent2D;

UCLASS(config=Game)
class AVehicleVisionPawn : public AWheeledVehicle
{
	GENERATED_BODY()

	/** Spring arm that will offset the camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** SCene component for the In-Car view origin */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* InternalCameraBase;

	/** Camera component for the In-Car view */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* InternalCamera;
	
public:
	AVehicleVisionPawn();


	/** Initial offset of incar camera */
	FVector InternalCameraOrigin;
	/** Initial rotation of incar camera */
	FRotator InternalCameraRotation;
	// Begin Pawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End Pawn interface

	// Begin Actor interface
	virtual void Tick(float Delta) override;
protected:
	virtual void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// End Actor interface

	/** Handle pressing forwards */
	void MoveForward(float Val);
	/** Update the physics material used by the vehicle mesh */
	void UpdatePhysicsMaterial();
	/** Handle pressing right */
	void MoveRight(float Val);
	/** Handle handbrake pressed */
	void OnHandbrakePressed();
	/** Handle handbrake released */
	void OnHandbrakeReleased();

private:
	/* Are we on a 'slippery' surface */
	bool bIsLowFriction;


public:
	/** Returns SpringArm subobject **/
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm; }
	/** Returns Camera subobject **/
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }
	/** Returns InternalCamera subobject **/
	FORCEINLINE UCameraComponent* GetInternalCamera() const { return InternalCamera; }

 public:
 	int FrameCounter = -1;
 	UTextureRenderTarget2D* MyRenderTarget;
 	USceneCaptureComponent2D* MySceneCapture2D;
 	TCPClientSocket* ClientSocket = nullptr;
 	bool SendImageToServer();
 	bool SaveRenderTargetToDisk();
 	int Width = 1000;
 	int Height = 400;
 	FString ServerMessage;
 	void LateralControl();
 	void LongitudinalControl();
 	int ValidTargetRange = 150;
 	float SteeringNormalizeFactor = 100.0;  // Higher number means less response to heading error
 	bool NextPressPauses = true;
 	float FOV = 120.f;
 	void PauseGame();
 	void ExitGame();
 	FString ReadFile(FString Filename);
 	bool SetConfig(FString FileString);
 	float TargetSpeed = 15.0;
 	bool AutonomousMode = false;
 	bool SaveImages = false;
 	bool AutonomousModeRequest = false;
 	bool ConnectedToServer = false;
};
