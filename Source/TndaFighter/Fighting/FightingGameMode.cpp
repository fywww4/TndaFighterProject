// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "Fighting/FightingGameMode.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "TndaFighter.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Misc/MessageDialog.h"
#endif

namespace
{
	/**
	 * 從世界中找出關卡內由設計師放置的 PlayerStart。
	 *
	 * 必須走 GetAllActorsOfClass 再逐一過濾，不能只用 GetActorOfClass：
	 * Editor 在 PIE 且關卡沒有 PlayerStart 時，會自動注入 APlayerStartPIE
	 * （繼承自 APlayerStart），GetActorOfClass 會誤判為有效出生點，導致 1P
	 * 出現在視角附近而防呆永遠不觸發。
	 */
	APlayerStart* FindLevelPlayerStart(const UObject* WorldContextObject)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(
			WorldContextObject, APlayerStart::StaticClass(), PlayerStarts);

		for (AActor* Candidate : PlayerStarts)
		{
			// 只接受關卡資產內的 APlayerStart；PIE 臨時 Actor 不列入 1P 出生點。
			if (IsValid(Candidate) && !Candidate->IsA<APlayerStartPIE>())
			{
				return Cast<APlayerStart>(Candidate);
			}
		}

		return nullptr;
	}

	/** 關卡缺少 PlayerStart 時的設計師提示；只在 1P 生成路徑呼叫，避免重複彈窗。 */
	void ReportMissingPlayerStart(const AFightingGameMode* GameMode)
	{
		FString LevelName = TEXT("Unknown");
		if (GameMode && GameMode->GetWorld())
		{
			LevelName = GameMode->GetWorld()->GetMapName();
		}

		/*
		 * MessageBox 會直接指出關卡與缺少的 Actor。
		 *
		 * 訊息會呈現為：
		 *    關卡：stage1
		 *    關卡設定錯誤：缺少 PlayerStart
		 *
		 *    請在關卡中放置一個 PlayerStart Actor（Place Actors > Basic > Player Start），
		 *    作為 1P 玩家出生點。每個戰鬥擂台只允許放置一個 PlayerStart。
		 */
		const FString ErrorMessage = FString::Printf(
			TEXT("關卡：%s\n關卡設定錯誤：缺少 PlayerStart\n\n請在關卡中放置一個 PlayerStart Actor（Place Actors > Basic > Player Start），作為 1P 玩家出生點。\n每個戰鬥擂台只允許放置一個 PlayerStart。"),
			*LevelName);
		UE_LOG(LogTndaFighter, Error, TEXT("%s"), *ErrorMessage);

#if WITH_EDITOR
		// 只在 Editor PIE 彈窗並結束 Play；打包版仍保留 Log 供 QA 追查。
		if (GEditor && GameMode && GameMode->GetWorld() && GameMode->GetWorld()->WorldType == EWorldType::PIE)
		{
			FMessageDialog::Open(
				EAppMsgCategory::Error,
				EAppMsgType::Ok,
				EAppReturnType::Ok,
				FText::FromString(ErrorMessage),
				NSLOCTEXT("FightingGameMode", "MissingPlayerStartTitle", "玩家出生點設定錯誤"));
			GEditor->RequestEndPlayMap();
		}
#endif
	}
}

AActor* AFightingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (APlayerStart* PlayerStart = FindPlayerSpawnPoint())
	{
		return PlayerStart;
	}

	ReportMissingPlayerStart(this);
	return nullptr;
}

APlayerStart* AFightingGameMode::FindPlayerSpawnPoint() const
{
	return FindLevelPlayerStart(this);
}
