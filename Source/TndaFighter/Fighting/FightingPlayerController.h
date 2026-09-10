// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FightingPlayerController.generated.h"

class UInputMappingContext;
class AFightingCameraActor;

/**
 * 本機格鬥玩家的控制中樞。
 *
 * 負責掛載 Enhanced Input Mapping Context，以及建立並持有共用格鬥攝影機。
 * 關卡再戰或 Pawn 重建流程不在此類處理，日後由 GameMode 或關卡流程另行評估。
 */
UCLASS(abstract, Config="Game")
class AFightingPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** 本機玩家啟用的 Enhanced Input Mapping Context；全部以優先權 0 加入。 */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** 僅由本機 Controller 擁有的執行期共用格鬥攝影機，不序列化到資產。 */
	UPROPERTY(Transient)
	TObjectPtr<AFightingCameraActor> FightingCamera;

	/** 可選的格鬥攝影機 Blueprint 子類；未指定時直接生成原生 `AFightingCameraActor`。 */
	UPROPERTY(EditDefaultsOnly, Category="Fighting Camera")
	TSubclassOf<AFightingCameraActor> FightingCameraClass;

protected:

	/** Controller 開始遊戲時提早建立攝影機，避免先顯示第三人稱鏡頭。 */
	virtual void BeginPlay() override;

	/** 遊戲結束時銷毀執行期攝影機，連帶解除攝影機註冊的 World 委派與遮罩。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 將預設 Mapping Context 掛到本機 LocalPlayer 的 Enhanced Input Subsystem。 */
	virtual void SetupInputComponent() override;

	/** Possess 新 Pawn 後確認格鬥攝影機已建立。 */
	virtual void OnPossess(APawn* InPawn) override;

public:

	/** 只有共用攝影機已取得玩家與 CPU，並成為 View Target 後才回傳 true。 */
	bool IsFightingCameraActive() const;

	/**
	 * 提供目前格鬥攝影機旋轉，供角色計算相對畫面的移動方向。
	 * 攝影機尚未啟用時回傳 false，且不修改 `OutRotation`。
	 */
	bool GetFightingCameraRotation(FRotator& OutRotation) const;

protected:

	/**
	 * 以可重複呼叫的方式，為本機 Controller 建立唯一的格鬥攝影機。
	 * 遠端 Controller、World 尚未就緒或攝影機已存在時不做任何事。
	 */
	void EnsureFightingCamera();

};
