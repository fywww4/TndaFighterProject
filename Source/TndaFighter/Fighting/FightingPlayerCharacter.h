// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "FightingPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UWidgetComponent;

/**
 * 本機玩家操作的格鬥角色。
 *
 * 目前原生類別負責移動輸入與相對格鬥攝影機的方向換算；其他戰鬥表現可由 Blueprint 子類接續實作。
 */
UCLASS(abstract)
class AFightingPlayerCharacter : public ACharacter
{
	GENERATED_BODY()
	
protected:

	/** 輸出 Vector2D 的 Enhanced Input 移動動作；X 為左右、Y 為前後。 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

public:
	
	/** 設定碰撞膠囊、移動速度與供其他系統辨識玩家的 Actor Tag。 */
	AFightingPlayerCharacter();

protected:

	/** 接收 Enhanced Input 值並轉送到共用的 `DoMove` 入口。 */
	void Move(const FInputActionValue& Value);

public:

	/**
	 * 處理硬體輸入或 UI 傳入的移動軸值。
	 * 方向以啟用中的格鬥攝影機為準；攝影機尚未就緒時退回 Controller Rotation。
	 */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

protected:

	/** 角色進入遊戲世界時的初始化入口；目前只保留父類生命週期。 */
	virtual void BeginPlay() override;

	/** 角色離開世界時的清理入口；目前只保留父類生命週期。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 將 `MoveAction` 的 Triggered 事件綁定到 `Move`。 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
