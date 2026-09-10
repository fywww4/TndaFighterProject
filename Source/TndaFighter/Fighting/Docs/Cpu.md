# CPU 對手模組

## 責任

`AFightingCpuSpawner` 在關卡開始時建立一名 CPU；`AFightingCpuCharacter` 提供 CPU 角色的原生基底；`AFightingCpuController` 保留日後接回 StateTree AI 的 Controller 基底。

原始碼：`FightingCpuSpawner.h/.cpp`、`FightingCpuCharacter.h/.cpp`、`FightingCpuController.h/.cpp`

## Spawner 設定與生成

關卡中的 `AFightingCpuSpawner` 使用膠囊元件標示出生位置與角色尺寸。方向箭頭只提供 Editor 視覺提示；CPU 成功生成後，Spawner 會向 `AFightingGameMode` 查詢擂台唯一的 1P `PlayerStart`，再讓 CPU 沿水平方向面向該出生點。

`Cpu Class` 必須指定有效且非抽象的 `AFightingCpuCharacter` 子類。Spawner 使用 `AdjustIfPossibleButAlwaysSpawn` 處理出生碰撞，盡量調整位置並生成 CPU。

## 設定錯誤

`Cpu Class` 未設定或不能生成時，Spawner 會執行下列處理：

- 以 Error 將 Spawner Blueprint 名稱與屬性路徑寫入 Output Log。
- 在 Editor PIE 顯示「CPU 生成設定錯誤」MessageBox，指出 `Class Defaults > Cpu Spawner > Cpu Class`。
- 訊息不顯示 World Partition UAID，並要求設計師指定有效且非抽象的 `AFightingCpuCharacter` 子類。
- 設計師按下「確定」後結束該次 PIE；打包版保留 Error Log，不顯示 Editor 對話框。

## 角色與 AI 狀態

`AFightingCpuCharacter` 將 `AFightingCpuController` 設為 AI Controller 類別，但 `AutoPossessAI` 預設為 `Disabled`。`BP_FightingCpuCharacter` 即使保留 `BP_FightingCpuController` 類別設定，生成後也不會自動建立或掛上 AI Controller，因此目前不會自主移動。

舊有的 `ST_CombatEnemy` 已移除；只供它使用的 `EnvQuery_Evade`、`EnvQuery_Fallback` 與 `EnvQuery_Flank` 也已刪除。`BP_FightingCpuController` 仍保留原有的 `StateTreeAIComponent`，但 `StateTreeRef` 與參數資料均為空。重新實作 AI 時，需要建立新的行為資產並重新指定。

## 目前限制

AI 停用設定會套用到所有 `AFightingCpuCharacter` 子類。Spawner 不監聽 CPU 死亡，也不會自動補生下一名對手。

`stage1` 目前仍放置關卡內的 `CPU_Spawner`。將 CPU 生成完全移入 C++ 並移除 Spawner 屬於另一項待完成調整。
