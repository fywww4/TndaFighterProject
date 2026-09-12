# CPU 對手模組

## 責任

`AFightingGameMode` 在玩家初始化完成後建立一名 CPU；`AFightingCpuCharacter` 提供 CPU 角色的原生基底；`AFightingCpuController` 保留日後接回 StateTree AI 的 Controller 基底。

原始碼：`FightingCpuCharacter.h/.cpp`、`FightingCpuController.h/.cpp`

## 出生設定與生成

關卡中的 `CpuStart` 是 Player Start Actor，`PlayerStartTag` 為 `CpuStart`。GameMode 使用它的 Transform 生成 CPU，並依雙方實際位置設定水平朝向；定位完成不等待落地。

GameMode 的 `Class Defaults > Fighting Match > Cpu Class` 必須指定有效且非抽象的 `AFightingCpuCharacter` 子類，使用 `AdjustIfPossibleButAlwaysSpawn` 處理出生碰撞。

## 設定錯誤

CPU 設定、出生點與生成錯誤統一交由 GameMode 回報；PIE 提示後停止對局，打包版保留 Error Log。完整契約見 [對局與出生](Match.md)。

## 角色與 AI 狀態

`AFightingCpuCharacter` 將 `AFightingCpuController` 設為 AI Controller 類別，但 `AutoPossessAI` 預設為 `Disabled`。`BP_FightingCpuCharacter` 即使保留 `BP_FightingCpuController` 類別設定，生成後也不會自動建立或掛上 AI Controller，因此目前不會自主移動。

舊有的 `ST_CombatEnemy` 已移除；只供它使用的 `EnvQuery_Evade`、`EnvQuery_Fallback` 與 `EnvQuery_Flank` 也已刪除。`BP_FightingCpuController` 仍保留原有的 `StateTreeAIComponent`，但 `StateTreeRef` 與參數資料均為空。重新實作 AI 時，需要建立新的行為資產並重新指定。

## 目前限制

AI 停用設定會套用到所有 `AFightingCpuCharacter` 子類。GameMode 不監聽 CPU 死亡，也不會自動補生下一名對手。
