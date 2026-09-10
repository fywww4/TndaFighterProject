# Fighting

`Fighting` 實作一名本機玩家對一名 CPU 的格鬥關卡流程。功能依責任分成四個文件模組；各文件記錄
介面、Editor 設定、失敗行為與目前限制。這些模組是文件與職責分組，原始碼仍屬於同一個
`TndaFighter` Unreal Module。

## 模組索引

| 模組 | 主要類別 | 文件 |
|---|---|---|
| 對局與出生 | `AFightingGameMode` | [Match](Docs/Match.md) |
| 玩家 | `AFightingPlayerController`、`AFightingPlayerCharacter` | [Player](Docs/Player.md) |
| 攝影機 | `AFightingCameraActor` | [Camera](Docs/Camera.md) |
| CPU 對手 | `AFightingCpuSpawner`、`AFightingCpuCharacter`、`AFightingCpuController` | [CPU](Docs/Cpu.md) |

## 啟動流程

1. `AFightingGameMode` 選取關卡內唯一的 1P `PlayerStart`，再由 Unreal 的玩家生成流程建立玩家角色。
2. `AFightingPlayerController` 掛載 Enhanced Input，建立格鬥攝影機並遮住尚未完成的構圖。
3. `AFightingCpuSpawner` 驗證 `CpuClass`、生成 CPU，並利用同一個 `PlayerStart` 設定 CPU 初始朝向。
4. `AFightingCameraActor` 取得玩家與 CPU 後切換 View Target，下一幀解除場景遮罩並恢復角色 UI。

原始碼註解與文件同步規則定義在 [Source 原始碼協作規則](../../AGENTS.md) 與本目錄的 [Fighting 原始碼協作規則](AGENTS.md)。
