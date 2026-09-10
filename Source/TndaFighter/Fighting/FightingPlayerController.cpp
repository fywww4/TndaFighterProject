// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "FightingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "FightingCameraActor.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

void AFightingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 本機 Controller 就緒時盡早建立攝影機，避免第一個遊戲畫面使用原本的第三人稱視角。
	EnsureFightingCamera();
}

void AFightingPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 銷毀攝影機時會一併解除 World Spawn 委派與尚未結束的啟動遮罩。
	if (IsValid(FightingCamera))
	{
		FightingCamera->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

void AFightingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input Subsystem 隸屬 LocalPlayer；遠端 Controller 沒有本機輸入，不應掛載 IMC。
	if (IsLocalPlayerController())
	{
		// 所有預設 Context 使用相同優先權；衝突規則交由各 Mapping Context 的設定處理。
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}

}

void AFightingPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 首次 Possess 與後續換 Pawn 的時序不同；此函式可重複呼叫且只會建立一次攝影機。
	EnsureFightingCamera();
}

bool AFightingPlayerController::IsFightingCameraActive() const
{
	return IsValid(FightingCamera) && FightingCamera->IsCameraActive();
}

bool AFightingPlayerController::GetFightingCameraRotation(FRotator& OutRotation) const
{
	// 啟動期間尚未定位完成時不改寫輸出，讓呼叫端保留 Controller 旋轉作為備援。
	if (!IsFightingCameraActive())
	{
		return false;
	}

	OutRotation = FightingCamera->GetViewRotation();
	return true;
}

void AFightingPlayerController::EnsureFightingCamera()
{
	// 只有本機 Controller 擁有 Viewport；遠端 Controller 不應重複建立攝影機。
	if (!IsLocalPlayerController() || IsValid(FightingCamera) || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	// Owner 用來表達執行期所有權；AlwaysSpawn 確保攝影機不受場景碰撞阻擋。
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 未指定 Blueprint 子類時仍使用原生 C++ Actor，保留既有不依賴資產的預設行為。
	TSubclassOf<AFightingCameraActor> CameraClass = FightingCameraClass;
	if (!CameraClass)
	{
		CameraClass = AFightingCameraActor::StaticClass();
	}
	FightingCamera = GetWorld()->SpawnActor<AFightingCameraActor>(
		CameraClass, FTransform::Identity, SpawnParameters);

	if (FightingCamera)
	{
		// 初始化會立刻加上黑幕、隱藏已存在的角色 UI，並開始等待 CPU 出現。
		FightingCamera->InitializeForController(this);
	}
}
