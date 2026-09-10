// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FightingCameraActor.generated.h"

class AFightingCpuCharacter;
class APlayerController;
class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class UWidgetComponent;

/**
 * 玩家與 CPU 共用的一對一格鬥攝影機。
 *
 * 初始化後先用黑幕與 Widget 隱藏啟動畫面，等場上同時存在玩家和最近的 CPU，才把自己
 * 設為 View Target。啟用後每幀追蹤兩人的水平軸與中點、平滑調整鏡頭距離，並限制玩家
 * 不得超出可構圖範圍。角色換邊時會選擇最接近目前 Yaw 的取景側，避免鏡頭翻轉 180 度。
 */
UCLASS(BlueprintType)
class AFightingCameraActor : public AActor
{
	GENERATED_BODY()

public:
	/** 建立相機根節點、SpringArm 與固定 16:9 輸出的 CameraComponent。 */
	AFightingCameraActor();

	/**
	 * 指定擁有此攝影機與玩家角色的 Controller，並重新開始等待對手的啟動流程。
	 * 呼叫後會立刻遮黑 3D 畫面、隱藏目前玩家 UI，並訂閱新 Actor 生成事件。
	 */
	void InitializeForController(APlayerController* InController);

	/** 完成第一次雙人構圖並設為 View Target 後回傳 true，直到重新初始化為止。 */
	bool IsCameraActive() const { return bCameraActive; }

	/** 回傳目前 CameraComponent 的世界旋轉；元件無效時退回 Actor Rotation。 */
	FRotator GetViewRotation() const;

protected:
	/** 在物理更新後限制玩家位置，接著更新雙人構圖與啟動遮罩狀態。 */
	virtual void Tick(float DeltaSeconds) override;

	/** 解除 World Spawn 委派、黑幕與本攝影機隱藏的 UI，再結束 Actor 生命週期。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 以最多每 0.25 秒一次的頻率，尋找距離玩家最近的有效 CPU。 */
	void RefreshOpponent();

	/**
	 * 將玩家限制在 CPU 周圍 `MaxFighterDistance` 的水平圓形範圍內。
	 * 超界時只移除向外的水平速度，保留向內、切線與垂直速度。
	 */
	void ConstrainPlayerToMaxDistance(AActor* PlayerActor, const AActor* OpponentActor) const;

	/** 在新對手的 Screen-space UI 首次渲染前先將其隱藏。 */
	void HandleActorSpawned(AActor* SpawnedActor);

	/**
	 * 固定攝影機就緒前，隱藏 Actor 身上目前可見的 WidgetComponent。
	 * 只記錄原本可見的元件，避免稍後誤開其他系統刻意隱藏的 UI。
	 */
	void HideVisibleWidgetComponents(AActor* Actor);

	/** 只恢復由本攝影機隱藏的 Actor UI。 */
	void RestoreHiddenWidgetComponents();

	/** 計算兩名角色中點，並把加上 `FocusHeight` 後的 Z 限制在初始焦點範圍內。 */
	FVector GetDesiredFocusLocation(const AActor* PlayerActor, const AActor* OpponentActor) const;

	/**
	 * 計算讓兩名角色連線在畫面上保持水平的鏡頭旋轉。
	 * 兩個候選 Yaw 相差 180 度，函式會選擇與目前鏡頭角度差較小的一側。
	 */
	FRotator GetDesiredCameraRotation(const AActor* PlayerActor, const AActor* OpponentActor) const;

	/** 依兩名角色完整水平距離，在最小與最大鏡頭距離之間做限制後的線性換算。 */
	float GetDesiredCameraDistance(const AActor* PlayerActor, const AActor* OpponentActor) const;

	/** 持續追蹤兩名角色中點的根節點。 */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> CameraRoot;

	/**
	 * 以旋轉與臂長將攝影機放在中點周圍；停用碰撞測試，避免場景物件把鏡頭推近而破壞構圖。
	 */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 實際輸出 55° FOV、固定 16:9 構圖的攝影機元件。 */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UCameraComponent> FightingCamera;

	/** 擁有 Viewport 與手動啟動遮罩的 Controller。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> OwningController;

	/** 持續取景時選中的最近 CPU。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AFightingCpuCharacter> Opponent;

	/** 由本攝影機隱藏，因此可安全恢復的 UI 元件。 */
	TArray<TWeakObjectPtr<UWidgetComponent>> HiddenWidgetComponents;

	/**
	 * 用於在新 CPU 首次渲染前遮住 UI 的 World Spawn 訂閱；EndPlay 或重新初始化時必須解除。
	 */
	FDelegateHandle ActorSpawnedHandle;

	/** 攝影機焦點相對角色原點向上的高度。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera", meta=(Units="cm"))
	float FocusHeight = 45.0f;

	/** 格鬥鏡頭固定使用的 Pitch、Roll；Yaw 只提供初始取景側，啟用後會依角色水平軸更新。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera")
	FRotator FixedCameraRotation = FRotator(-10.0f, 0.0f, 0.0f);

	/** 兩名角色重疊或很接近時使用的最小鏡頭距離。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Distance", meta=(ClampMin="100", Units="cm"))
	float MinCameraDistance = 440.0f;

	/** 兩名角色到達最大構圖間距時使用的鏡頭距離上限值；實際值不會小於 `MinCameraDistance`。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Distance", meta=(ClampMin="100", Units="cm"))
	float MaxCameraDistance = 690.0f;

	/** 開始由最小鏡頭距離拉遠的兩名角色水平距離。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Distance", meta=(ClampMin="0", Units="cm"))
	float MinFighterDistance = 110.0f;

	/**
	 * 到達最大鏡頭距離的兩名角色水平距離，同時也是玩家可離開 CPU 的最大水平距離。
	 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Distance", meta=(ClampMin="1", Units="cm"))
	float MaxFighterDistance = 570.0f;

	/** 鏡頭臂長的插值速度；值越高越快，設為 0 時會直接套用目標距離。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Smoothing", meta=(ClampMin="0"))
	float DistanceInterpolationSpeed = 5.0f;

	/** 相對初始焦點允許追蹤的最大垂直偏移。 */
	UPROPERTY(EditAnywhere, Category="Fighting Camera|Focus", meta=(ClampMin="0", Units="cm"))
	float MaxVerticalFocusOffset = 250.0f;

	/** 下次允許搜尋對手的世界時間，用來限制輪詢頻率。 */
	float NextOpponentSearchTime = 0.0f;

	/** 第一次取得玩家與 CPU、完成初始構圖並設定 View Target 後為 true。 */
	bool bCameraActive = false;

	/** 延後解除遮罩，直到格鬥 View Target 已完整渲染一幀。 */
	bool bPendingFadeRelease = false;

	/** 第一次完成構圖時的焦點高度，用來限制跳躍造成的垂直追蹤。 */
	float InitialFocusZ = 0.0f;
};
