# 對局與出生模組

## 責任

`AFightingGameMode` 是格鬥關卡的伺服器端規則入口。它固定以一名本機玩家對一名 CPU 為前提，讓 Unreal 的標準玩家生成流程建立 1P，並提供 CPU 查詢同一個出生點。

原始碼：`FightingGameMode.h`、`FightingGameMode.cpp`

## 關卡設定

每個戰鬥擂台必須由設計師放置一個 1P `PlayerStart`。`AFightingGameMode::ChoosePlayerStart()` 固定選用這個 Actor，不採用 Editor PIE 自動注入的 `PlayerStartPIE`。

`FindPlayerSpawnPoint()` 只查詢關卡內有效的 `PlayerStart`，不顯示錯誤。`AFightingCpuSpawner` 會在 CPU 生成後呼叫此函式，以相同出生點設定 CPU 的初始水平朝向。

## 失敗行為

關卡缺少有效 `PlayerStart` 時，`ChoosePlayerStart()` 會執行下列處理：

- 以 Error 將關卡名稱與設定說明寫入 Output Log。
- 在 Editor PIE 顯示「玩家出生點設定錯誤」MessageBox，提示設計師從 `Place Actors > Basic > Player Start` 放置 Actor。
- 設計師按下「確定」後結束該次 PIE；打包版保留 Error Log，不顯示 Editor 對話框。

錯誤由玩家生成路徑統一回報。CPU 查詢出生點時不會重複彈出對話框。

## 目前限制

本模組只支援一個 1P 出生點，尚未處理多名本機玩家、回合重置或多出生點選擇。
