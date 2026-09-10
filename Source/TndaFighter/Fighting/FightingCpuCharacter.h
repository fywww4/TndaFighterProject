// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Engine/TimerHandle.h"
#include "FightingCpuCharacter.generated.h"

/** 預留給 StateTree：CPU 攻擊動畫播放完畢時通知等待中的任務。 */
DECLARE_DELEGATE(FOnEnemyAttackCompleted);

/** 預留給 StateTree：CPU 從空中落地時通知等待中的任務。 */
DECLARE_DELEGATE(FOnEnemyLanded);

/** CPU 死亡時供 Blueprint 與其他執行期系統訂閱的多播事件型別。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDied);

/**
 * CPU 對手的原生角色基底。
 *
 * 建構時指定 `AFightingCpuController`，但目前將 Auto Possess AI 設為 Disabled，
 * 因此生成後不會自動建立 Controller 或執行 StateTree。原生類別只保留碰撞尺寸與
 * 朝向移動方向的基本設定，供 Blueprint 子類接續擴充外觀與戰鬥表現。
 */
UCLASS(abstract)
class AFightingCpuCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	/** 設定 AI Controller 類別、暫停自動接管，並初始化碰撞與旋轉方式。 */
	AFightingCpuCharacter();


protected:

	/** CPU 進入遊戲世界時的初始化入口；目前只保留父類生命週期。 */
	virtual void BeginPlay() override;

	/** CPU 離開世界時的清理入口；目前只保留父類生命週期。 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
};
