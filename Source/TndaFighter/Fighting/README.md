# Fighting

`Fighting` 實作一名本機玩家對一名 CPU 的格鬥關卡流程。功能依責任分成五個文件模組；各文件記錄
介面、Editor 設定、失敗行為與目前限制。這些模組是文件與職責分組，原始碼仍屬於同一個
`TndaFighter` Unreal Module。

## 模組索引

| 模組 | 主要類別 | 文件 |
|---|---|---|
| 對局與出生 | `AFightingGameMode` | [Match](Docs/Match.md) |
| 玩家 | `AFightingPlayerController`、`AFightingPlayerCharacter` | [Player](Docs/Player.md) |
| 攝影機 | `AFightingCameraActor` | [Camera](Docs/Camera.md) |
| CPU 對手 | `AFightingCpuCharacter`、`AFightingCpuController` | [CPU](Docs/Cpu.md) |
| 動畫資料 | `UFightingAnimInstance` | [Animation](Docs/Animation.md) |

## 啟動流程

1. `AFightingGameMode` 檢查 `PlayerStart`、`CpuStart` 兩個 Tag 的唯一出生點，由 Unreal 標準流程建立並 Possess 玩家。
2. `AFightingPlayerController` 掛載 Enhanced Input，並停用自動選鏡。
3. `AFightingGameMode` 在標準玩家初始化完成後，於 `CpuStart` 生成 CPU，再依雙方實際位置設定面向彼此的初始朝向。
4. `AFightingGameMode` 建立相機並傳入雙方角色；相機同步完成初始構圖與 SpringArm 插槽更新後，GameMode 才切換 View Target，直接呈現完整格鬥畫面。

原始碼註解與文件同步規則定義在 [Source 原始碼協作規則](../../AGENTS.md) 與本目錄的 [Fighting 原始碼協作規則](AGENTS.md)。
