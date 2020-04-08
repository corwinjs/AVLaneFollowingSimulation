#include "VehicleVisionPawn.h"
#include "VehicleVisionWheelFront.h"
#include "VehicleVisionWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "GameFramework/Controller.h"
#include "Runtime/Engine/Classes/Components/SceneCaptureComponent2D.h"
#include "Runtime/Engine/Classes/Engine/SceneCapture2D.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "PixelFormat.h"
#include "CoreGlobals.h"
#include "Components/ActorComponent.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "HAL/PlatformFilemanager.h"

#include "Runtime/Engine/Public/HighResScreenshot.h"
#include "Runtime/ImageWriteQueue/Public/ImageWriteBlueprintLibrary.h"
#include "ImageWriteTask.h"
#include "ImageWriteQueue.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

AVehicleVisionPawn::AVehicleVisionPawn() {
	// Car mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT("/Game/Vehicle/Sedan/Sedan_SkelMesh.Sedan_SkelMesh"));
	GetMesh()->SetSkeletalMesh(CarMesh.Object);

	static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(TEXT("/Game/Vehicle/Sedan/Sedan_AnimBP"));
	GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
	
	// Simulation
	UWheeledVehicleMovementComponent4W* Vehicle4W = CastChecked<UWheeledVehicleMovementComponent4W>(GetVehicleMovement());

	check(Vehicle4W->WheelSetups.Num() == 4);

	Vehicle4W->WheelSetups[0].WheelClass = UVehicleVisionWheelFront::StaticClass();
	Vehicle4W->WheelSetups[0].BoneName = FName("Wheel_Front_Left");
	Vehicle4W->WheelSetups[0].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[1].WheelClass = UVehicleVisionWheelFront::StaticClass();
	Vehicle4W->WheelSetups[1].BoneName = FName("Wheel_Front_Right");
	Vehicle4W->WheelSetups[1].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	Vehicle4W->WheelSetups[2].WheelClass = UVehicleVisionWheelRear::StaticClass();
	Vehicle4W->WheelSetups[2].BoneName = FName("Wheel_Rear_Left");
	Vehicle4W->WheelSetups[2].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[3].WheelClass = UVehicleVisionWheelRear::StaticClass();
	Vehicle4W->WheelSetups[3].BoneName = FName("Wheel_Rear_Right");
	Vehicle4W->WheelSetups[3].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->TargetOffset = FVector(0.f, 0.f, 200.f);
	SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 7.f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;

	InternalCameraOrigin = FVector(210.0f, 0, 220.0f);
	InternalCameraRotation = FRotator(-30.0f, 0.0f, 0.0f);

	InternalCameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("InternalCameraBase"));
	InternalCameraBase->SetRelativeLocation(InternalCameraOrigin);
	InternalCameraBase->SetRelativeRotation(InternalCameraRotation);
	InternalCameraBase->SetupAttachment(GetMesh());

	InternalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InternalCamera"));
	InternalCamera->bUsePawnControlRotation = false;
	InternalCamera->SetFieldOfView(FOV);
	InternalCamera->SetupAttachment(InternalCameraBase);

	MySceneCapture2D = CreateDefaultSubobject<USceneCaptureComponent2D>(
                                                           TEXT("MySceneCapture2D"));
	MySceneCapture2D->AttachToComponent(InternalCamera, FAttachmentTransformRules
                                       (EAttachmentRule::KeepRelative, false));
	MySceneCapture2D->FOVAngle = InternalCamera->FieldOfView;
	MyRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("MyRenderTarget"));
	EPixelFormat PixelFormat = EPixelFormat::PF_A16B16G16R16;
	bool bInForceLinearGamma = false;
	MyRenderTarget->InitCustomFormat(uint32(Width),
                                         uint32(Height),
                                         PixelFormat,
                                         bInForceLinearGamma);
	MySceneCapture2D->TextureTarget = MyRenderTarget;
}

void AVehicleVisionPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AVehicleVisionPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AVehicleVisionPawn::MoveRight);

	PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &AVehicleVisionPawn::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &AVehicleVisionPawn::OnHandbrakeReleased);

	FInputActionBinding& PauseBinding = PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &AVehicleVisionPawn::PauseGame);
	PauseBinding.bExecuteWhenPaused = true;

	FInputActionBinding& ExitBinding = PlayerInputComponent->BindAction("Exit", IE_Pressed, this, &AVehicleVisionPawn::ExitGame);
	ExitBinding.bExecuteWhenPaused = true;
}

void AVehicleVisionPawn::MoveForward(float Val) {
	GetVehicleMovementComponent()->SetThrottleInput(Val);
}

void AVehicleVisionPawn::MoveRight(float Val) {
	GetVehicleMovementComponent()->SetSteeringInput(Val);
}

void AVehicleVisionPawn::OnHandbrakePressed() {
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AVehicleVisionPawn::OnHandbrakeReleased() {
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void AVehicleVisionPawn::Tick(float Delta) {
	Super::Tick(Delta);
	++FrameCounter;

	if (SaveImages) {
		SaveRenderTargetToDisk();
	}

	if (AutonomousMode) {
		if (!ClientSocket->IsConnected()) ExitGame();
		SendImageToServer();
		LongitudinalControl();
		LateralControl();
	}
}

void AVehicleVisionPawn::BeginPlay()
{
	Super::BeginPlay();

	FString ConfigString = "";
	ConfigString = ReadFile("config.txt");
	if (ConfigString == "") {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("config.txt not found - using default configuration"));
	}
	if (!SetConfig(ConfigString)) ExitGame();

	if (AutonomousModeRequest) {
		ClientSocket = new TCPClientSocket(1024);
		int Result = ClientSocket->MakeConnection("127.0.0.1", 27015);
		if (Result == 0) {
			UE_LOG(LogTemp, Log, TEXT("Successful connection to server"));
			ConnectedToServer = true;
		} else {
			UE_LOG(LogTemp, Warning, TEXT("Unable to connect to server"));
			ConnectedToServer = false;
		}
	}

	if (ConnectedToServer && AutonomousModeRequest) {
		AutonomousMode = true;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Server Detected - Autonomous Drive Mode"));
	} else if (!ConnectedToServer && AutonomousModeRequest) {
		AutonomousMode = false;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Server Detected - Manual Drive Mode"));
	} else {
		AutonomousMode = false;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Manual Drive Mode Selected"));
	}
}

void AVehicleVisionPawn::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	if (AutonomousMode) ClientSocket->CloseSocket();
}

void AVehicleVisionPawn::PauseGame() {
	UGameplayStatics::SetGamePaused(GetWorld(), NextPressPauses);
	NextPressPauses = !NextPressPauses;
}

void AVehicleVisionPawn::ExitGame() {
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

bool AVehicleVisionPawn::SendImageToServer() {
    FTextureRenderTargetResource* RTResource = MyRenderTarget->GameThread_GetRenderTargetResource();

    FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
    ReadPixelFlags.SetLinearToGamma(false);

    TArray<FColor> OutBMP;
    RTResource->ReadPixels(OutBMP, ReadPixelFlags);
    uint8* OutBMPGray = new uint8[Width*Height];

    float R_contribution = .3;
    float G_contribution = .59;
    float B_contribution = .11;
    FColor Color;
    for (int i = 0; i < OutBMP.Num(); i++) {
    	Color = OutBMP[i];
    	OutBMPGray[i] = Color.R*R_contribution + Color.G*G_contribution + Color.B*B_contribution;
    }

	ClientSocket->SendBytes(OutBMPGray, Width*Height);
	ClientSocket->ReadMessageIntoBuffer(true);
	ServerMessage = ClientSocket->GetStringFromBuffer();
    
    return true;
}

bool AVehicleVisionPawn::SaveRenderTargetToDisk() {
    FTextureRenderTargetResource* RTResource = MyRenderTarget->GameThread_GetRenderTargetResource();
    FString OutputDirectory = FPaths::ProjectDir() + "/output/";
    FString FileName = OutputDirectory + FString("Images/") + "Camera" + FString("_") +
    				FString::FromInt(FrameCounter) + FString(".png");

    FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
    ReadPixelFlags.SetLinearToGamma(false);

    TArray<FColor> OutBMP;
    RTResource->ReadPixels(OutBMP, ReadPixelFlags);

    for (FColor& color : OutBMP) {
        color.A = 255;
    }

    FIntPoint DestSize(MyRenderTarget->GetSurfaceWidth(), MyRenderTarget->GetSurfaceHeight());

    UE_LOG(LogTemp, Log, TEXT("Saving image to %s"), *FileName);

    FString ResultPath;
    FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();

    TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
    ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(DestSize, MoveTemp(OutBMP));

    HighResScreenshotConfig.PopulateImageTaskParams(*ImageTask);
    ImageTask->Filename = FileName;

    TFuture<bool> CompletionFuture =
                 HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
    return true;
}

void AVehicleVisionPawn::LateralControl() {
	TArray<FString> Coordinates;
	ServerMessage.ParseIntoArray(Coordinates, TEXT(","), true);
	FString ColumnString = Coordinates[1];
	int ColumnInt = FCString::Atoi(*ColumnString);
	int ErrorVal = 0;
	if (ColumnInt < Width/2 + ValidTargetRange && ColumnInt > Width/2 - ValidTargetRange) {
		ErrorVal = ColumnInt - Width/2;
	}

	float NormalizedError = ErrorVal/SteeringNormalizeFactor;
	GetVehicleMovementComponent()->SetSteeringInput(NormalizedError);
}

void AVehicleVisionPawn::LongitudinalControl() {
	float CommandedSpeed = TargetSpeed;
	float CurrentSpeed = GetVehicleMovementComponent()->GetForwardSpeed() / 44.704;

    float velocityError = CommandedSpeed - CurrentSpeed;
    float ControlInput = 0.2 * velocityError + 0.015 * CommandedSpeed;
    if (ControlInput <= 0) {
        GetVehicleMovementComponent()->SetThrottleInput(0);
        GetVehicleMovementComponent()->SetHandbrakeInput(true);
    } else if (ControlInput > 1) {
        GetVehicleMovementComponent()->SetThrottleInput(1);
        GetVehicleMovementComponent()->SetHandbrakeInput(false);
    } else {
        GetVehicleMovementComponent()->SetThrottleInput(ControlInput);
        GetVehicleMovementComponent()->SetHandbrakeInput(false);
    }
}

FString AVehicleVisionPawn::ReadFile(FString Filename) {
    FString Directory = FPaths::LaunchDir();
	if (Directory.Len() < 3) Directory = FPaths::ProjectDir();
	FString Result;
	IPlatformFile& File = FPlatformFileManager::Get().GetPlatformFile();
	if (File.CreateDirectory(*Directory)) {
		FString MyFile = Directory + "/" + Filename;
		FFileHelper::LoadFileToString(Result, *MyFile);
	}
	return Result;
}

bool AVehicleVisionPawn::SetConfig(FString FileString) {
	TArray<FString> Lines;
	FileString.ParseIntoArray(Lines, TEXT("\n"), true);
	for (FString Line : Lines) {
		if (Line.Len() > 1 && Line[0] != '#') {
			TArray<FString> Segments;
			Line.ParseIntoArray(Segments, TEXT(":"), true);
			FString Key = Segments[0];
			FString Value = Segments[1];
			if (Key == "Width") Width = FCString::Atoi(*Value);
			else if (Key == "Height") Height = FCString::Atoi(*Value);
			else if (Key == "ValidTargetRange") ValidTargetRange = FCString::Atoi(*Value);
			else if (Key == "FOV") FOV = FCString::Atof(*Value);
			else if (Key == "TargetSpeed") TargetSpeed = FCString::Atof(*Value);
			else if (Key == "SteeringNormalizeFactor") SteeringNormalizeFactor = FCString::Atof(*Value);
			else if (Key == "CameraPlacementX") InternalCameraOrigin.X = FCString::Atof(*Value);
			else if (Key == "CameraPlacementZ") InternalCameraOrigin.Z = FCString::Atof(*Value);
			else if (Key == "CameraPlacementPitch") InternalCameraRotation.Pitch = FCString::Atof(*Value);
			else if (Key == "SaveImages") SaveImages = (Value.Contains("true"));
			else if (Key == "AutonomousMode") AutonomousModeRequest = (Value.Contains("true"));
		}
	}

	InternalCameraBase->SetRelativeLocation(InternalCameraOrigin);
	InternalCameraBase->SetRelativeRotation(InternalCameraRotation);
	MyRenderTarget->ResizeTarget(uint32(Width), uint32(Height));
	InternalCamera->SetFieldOfView(FOV);
	MySceneCapture2D->FOVAngle = InternalCamera->FieldOfView;

	return true;
}

#undef LOCTEXT_NAMESPACE
