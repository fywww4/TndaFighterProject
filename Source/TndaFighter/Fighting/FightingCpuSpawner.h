// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FightingCpuSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class AFightingCpuCharacter;

/**
 * 關卡內用來生成一名 CPU 對手的定位 Actor。
 *
 * `BeginPlay` 會在膠囊元件標示的 Transform 生成 `CpuClass`，再讓角色水平朝向玩家 1P
 * 實際採用的 PlayerStart。目前不監聽 CPU 死亡，也不會自動補生下一名對手。
 */
UCLASS(abstract)
class AFightingCpuSpawner : public AActor
{
	GENERATED_BODY()
	
	/** 預覽 CPU 出生位置與角色碰撞尺寸；執行時不參與碰撞。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	/** 在關卡編輯器中提示 Spawner 朝向的視覺元件，不直接決定 CPU 最終朝向。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

protected:

	/** 遊戲開始時要生成的 CPU 角色類別；Editor 中設定無效時會顯示錯誤對話框並停止 PIE。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cpu Spawner")
	TSubclassOf<AFightingCpuCharacter> CpuClass;

public:	
	
	/** 建立出生位置的膠囊與方向提示元件。 */
	AFightingCpuSpawner();

public:

	/** 驗證並生成一名 CPU；設定錯誤時提示設計師並停止 PIE，成功生成後再把角色轉向 1P。 */
	virtual void BeginPlay() override;

	/** Spawner 離開世界時的清理入口；目前只保留父類生命週期。 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

};
