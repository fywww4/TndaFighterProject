// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FightingGameMode.generated.h"

class APlayerStart;

/**
 * 格鬥關卡的伺服器端規則入口。
 *
 * 遊戲固定為一名本機玩家對一名 CPU，玩家出生交由 Unreal 的預設流程處理。
 * 每個戰鬥擂台只允許放置一個 PlayerStart，作為1P玩家控制角色出生點。
 */
UCLASS(abstract)
class AFightingGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	/**
	 * 尋找擂台內唯一的 1P 出生點，供 CPU 設定初始朝向。
	 *
	 * 只接受關卡內設計師放置的 `APlayerStart`；Editor PIE 自動注入的
	 * `APlayerStartPIE` 不計入。此函式只做查詢，不彈錯誤視窗。
	 * @return 關卡內的 PlayerStart；未放置時回傳 nullptr。
	 */
	APlayerStart* FindPlayerSpawnPoint() const;

	/**
	 * 1P 生成時固定走 `FindPlayerSpawnPoint()`，不走 Engine 預設的 PIE 備援出生點。
	 * 缺少 PlayerStart 時在此寫 Log、彈 MessageBox 並結束 PIE；由 `RestartPlayer`
	 * 在 Actor BeginPlay 之前呼叫，是玩家出生防呆的唯一入口。
	 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
