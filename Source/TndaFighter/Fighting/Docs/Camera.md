# 攝影機模組

## 責任

`AFightingCameraActor` 提供玩家與 CPU 共用的一對一格鬥視角。它接收 GameMode 指定的玩家與 CPU，負責初始構圖、後續追蹤與玩家最大水平間距。

原始碼：`FightingCameraActor.h`、`FightingCameraActor.cpp`

## 建立與啟用

`AFightingGameMode` 在雙方生成及初始朝向完成後建立一個 `AFightingCameraActor`，Owner 為 GameMode，隨關卡世界結束清除。若要在 Blueprint 調整構圖參數，可建立相機子類並指定給 GameMode 的 `Fighting Match > FightingCameraClass`；未指定時使用原生 C++ 類別。

`InitializeForFighters(Player, Cpu)` 只接受已生成的有效角色。它以出生碰撞調整後的實際位置設定焦點、初始高度、旋轉與臂長；首次不插值。直接修改臂長不會更新 SpringArm 插槽，因此初始化會暫時略過 SpringArm Lag，同步更新一次插槽及 CameraComponent，再由 GameMode 指定 View Target 與 Camera Cut。

## 雙人構圖

攝影機在 `PostPhysics` Tick，SpringArm 以 Actor Tick 為前置條件，使用同幀更新後的構圖。每幀依角色完成移動與物理更新後的位置執行下列工作：

- 精確追蹤玩家與 CPU 的世界座標中點。
- 讓鏡頭 Yaw 與雙方水平連線保持垂直，使兩名角色在畫面上的站位線維持水平。
- 把角色連線視為沒有固定方向的軸；雙方交換左右時只交換畫面站位，不讓鏡頭翻轉 180 度。
- 依雙方完整水平 2D 距離平滑調整 SpringArm 臂長。

FOV 固定為 `55°`。SpringArm Collision Test 保持關閉，避免場景碰撞改變角色畫面占比。

攝影機將可見遊戲區固定為 16:9。執行視窗使用其他比例時，Unreal 會以黑邊保留構圖，不拉伸畫面，也不改變作業系統視窗的像素尺寸。此設定只隨 `AFightingCameraActor` 生效，目前套用於 `stage1` 的格鬥視角。

## 可調參數

參數位於 `FightingCameraActor.h` 的 `Fighting Camera` 類別屬性。

| 屬性 | 預設值 | 用途 |
|---|---:|---|
| `FocusHeight` | `45 cm` | 將焦點抬高到角色原點上方。 |
| `FixedCameraRotation` | Pitch `-10°`、Yaw `0°`、Roll `0°` | Pitch 與 Roll 固定使用；Yaw 只決定初始取景側。 |
| `MinCameraDistance` | `440 cm` | 雙方接近時的 SpringArm 臂長。 |
| `MaxCameraDistance` | `690 cm` | 雙方到達最大構圖間距時的臂長。 |
| `MinFighterDistance` | `110 cm` | 開始將鏡頭由最近距離拉遠的角色間距。 |
| `MaxFighterDistance` | `570 cm` | 到達最遠鏡頭距離的角色間距，也是 1P 的水平移動邊界。 |
| `DistanceInterpolationSpeed` | `5` | SpringArm 臂長的插值速度。 |
| `MaxVerticalFocusOffset` | `250 cm` | 焦點相對初始高度可上下追蹤的最大距離。 |

距離端點以示範圖的最近與最遠角色畫面占比為基準，並以 `stage1` 的實際 Skeletal Mesh Bounds 投影校正。兩名角色的水平距離會在 `110–570 cm` 之間線性對應到 `440–690 cm` 的鏡頭距離。

## 玩家移動邊界

攝影機在取景前將 1P 限制於 CPU 周圍 `MaxFighterDistance` 的水平圓形範圍。玩家超界時，攝影機會校正水平位置並移除遠離 CPU 的速度分量；向 CPU 靠近、沿邊界側移與垂直跳躍不受影響。

`AFightingPlayerCharacter` 會向 PlayerController 查詢實際攝影機旋轉，作為相對畫面的移動方向。輸入與動畫行為記錄於 [玩家模組](Player.md)。

## 生命週期與首幀

GameMode 在雙方定位後同步完成相機初始化，Controller 停用自動管理 View Target。沒有搜尋、輪詢、World Spawn 委派、啟動黑幕、Widget 隱藏／恢復或延後解除遮罩；角色 UI 保持原本的可見設定。

相機保存雙方弱參照，任何一方失效時停止更新，不搜尋替代對手，也不處理角色生成、重生或回合重置。
