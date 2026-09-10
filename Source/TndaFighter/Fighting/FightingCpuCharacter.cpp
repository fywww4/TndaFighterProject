// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "FightingCpuCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FightingCpuController.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

AFightingCpuCharacter::AFightingCpuCharacter()
{
	// Blueprint 子類未覆寫時，日後重新啟用 AI 會使用專案的格鬥 CPU Controller。
	AIControllerClass = AFightingCpuController::StaticClass();

	// 保留 AI Controller 類別供後續階段使用，但目前不自動建立 Controller。
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Actor 不直接複製 Controller Yaw，改由 CharacterMovement 的 Desired Rotation 平順轉身。
	bUseControllerRotationYaw = false;


	// 對齊玩家角色的膠囊尺寸，維持雙方接觸與出生高度的一致基準。
	GetCapsuleComponent()->SetCapsuleSize(35.0f, 90.0f);

	// AI 恢復後，移動元件會朝 Controller 的期望旋轉轉向，不需要直接鎖 Actor Yaw。
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AFightingCpuCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFightingCpuCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
