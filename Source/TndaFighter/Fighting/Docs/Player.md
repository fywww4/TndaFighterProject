# 玩家模組

## 責任

`AFightingPlayerController` 負責本機玩家的 Enhanced Input 與格鬥攝影機生命週期；`AFightingPlayerCharacter` 接收移動輸入，並把輸入方向換算到目前畫面座標。

原始碼：`FightingPlayerController.h/.cpp`、`FightingPlayerCharacter.h/.cpp`

## Controller 設定

`DefaultMappingContexts` 內的 Enhanced Input Mapping Context 會以優先權 `0` 加入本機 `LocalPlayer`。遠端 Controller 不會掛載本機輸入。

`AFightingPlayerController` 會在 `BeginPlay()` 或 Possess 新 Pawn 時確認格鬥攝影機已建立。本機 Controller 只持有一個執行期攝影機；若 `FightingCameraClass` 未指定 Blueprint 子類，Controller 會直接生成原生 `AFightingCameraActor`。Controller 結束遊戲時會銷毀該攝影機。

## 移動輸入

`MoveAction` 輸出 `Vector2D`，X 代表左右，Y 代表前後。`AFightingPlayerCharacter::DoMove()` 讓硬體輸入與 Blueprint／UI 共用同一套方向換算。

共用攝影機啟用後，移動方向以實際攝影機的水平旋轉為準；攝影機尚未取得玩家與 CPU 時，角色改用 Controller Rotation。兩種路徑都只使用 Yaw，避免攝影機俯角產生垂直移動分量。

共用攝影機啟用期間，Look 輸入不修改 Controller Rotation，避免操作方向與畫面脫節。

## 受阻時的移動動畫

`ABP_Manny_Combat` 的 locomotion 優先使用角色真實 Velocity。角色仍有移動輸入，但因 CPU 碰撞或格鬥攝影機邊界使真實速度低於 `Blocked Movement Animation Speed` 時，AnimBP 會改用輸入加速度方向與此速度產生動畫用 Velocity。

`Ground Speed`、`Should Move` 與 `Direction` 都由動畫用 Velocity 計算。`Blocked Movement Animation Speed` 預設為 `300 cm/s`，對應目前 BlendSpace 的 Walk sample；玩家頂住 CPU 或攝影機邊界時仍會維持移動動作，放開輸入後會回到 Idle。

## 目前限制

`AFightingPlayerController` 不監聽 Pawn 銷毀，也不會在 Pawn 被移除後自動重生。關卡再戰、回合重置或 KO 後重建角色，日後由 GameMode 或關卡流程另行設計。
