# 玩家模組

## 責任

`AFightingPlayerController` 負責本機玩家的 Enhanced Input 與實際格鬥視角查詢；`AFightingPlayerCharacter` 接收移動輸入，並把輸入方向換算到目前畫面座標。

原始碼：`FightingPlayerController.h/.cpp`、`FightingPlayerCharacter.h/.cpp`

## Controller 設定

`DefaultMappingContexts` 內的 Enhanced Input Mapping Context 會以優先權 `0` 加入本機 `LocalPlayer`。遠端 Controller 不會掛載本機輸入。

`AFightingPlayerController` 在建構時停用 `bAutoManageActiveCameraTarget`，避免標準玩家初始化覆寫 GameMode 指定的視角。相機建立及類別設定由 [GameMode](Match.md) 負責；Controller 不保存另一份相機參照，`IsFightingCameraActive()` 與 `GetFightingCameraRotation()` 直接查詢實際 View Target。

## 移動輸入

`MoveAction` 輸出 `Vector2D`，X 代表左右，Y 代表前後。`AFightingPlayerCharacter::DoMove()` 讓硬體輸入與 Blueprint／UI 共用同一套方向換算。

共用攝影機啟用後，移動方向以實際攝影機的水平旋轉為準；View Target 不是有效格鬥相機時，角色改用 Controller Rotation。兩種路徑都只使用 Yaw，避免攝影機俯角產生垂直移動分量。

目前原生類別只綁定 MoveAction，Blueprint EventGraph 為空；未綁定 Look 或跳躍輸入。跳躍構圖驗證使用既有 `ACharacter::Jump()`，本次不新增輸入功能。

## 受阻時的移動動畫

`UFightingAnimInstance` 負責 `ABP_Manny_Combat` 的 locomotion 資料。原生類別優先使用角色真實 Velocity；角色仍有移動輸入，但因 CPU 碰撞或格鬥攝影機邊界使真實速度低於 `Blocked Movement Animation Speed` 時，會改用輸入加速度方向與此速度產生動畫用 Velocity。AnimBP 只讀取結果，不再用 EventGraph 重複計算。

`Ground Speed`、`Should Move` 與 `Direction` 都由動畫用 Velocity 計算。`Blocked Movement Animation Speed` 預設為 `300 cm/s`，對應目前 BlendSpace 的 Walk sample，可在 `ABP_Manny_Combat` 的 Class Defaults 調整；玩家頂住 CPU 或攝影機邊界時仍會維持移動動作，放開輸入後會回到 Idle。欄位與生命週期細節見 [動畫資料模組](Animation.md)。

## 目前限制

`AFightingPlayerController` 不監聽 Pawn 銷毀，也不會在 Pawn 被移除後自動重生。關卡再戰、回合重置或 KO 後重建角色，日後由 GameMode 或關卡流程另行設計。
