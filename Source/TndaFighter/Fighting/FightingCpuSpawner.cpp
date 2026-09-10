// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "FightingCpuSpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "FightingCpuCharacter.h"
#include "FightingGameMode.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Misc/MessageDialog.h"
#endif
#include "TndaFighter.h"

AFightingCpuSpawner::AFightingCpuSpawner()
{
	// 獨立根節點保留 Actor Transform，讓定位膠囊和方向箭頭共用同一個基準。
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 膠囊只作為出生 Transform 與角色尺寸預覽，不應阻擋場景中的其他物件。
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	// 箭頭只提供編輯器中的朝向提示；實際 CPU 會在生成後轉向玩家出生點。
	SpawnDirection->SetupAttachment(RootComponent);
}

void AFightingCpuSpawner::BeginPlay()
{
	Super::BeginPlay();

	const bool bHasSpawnableCpuClass = IsValid(CpuClass)
		&& !CpuClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
	if (!bHasSpawnableCpuClass)
	{
		FString SpawnerBlueprintName = GetClass()->GetName();

		/*
		 * MessageBox 現在會直接指出 Blueprint 與未設定的屬性。
		 * 
		 * 訊息會呈現為：
		 *    Blueprint：BP_CombatEnemySpawner
		 *    屬性設定錯誤：Class Defaults > Cpu Spawner > Cpu Class
		 *    
		 *    請將 Cpu Class 指定為有效且非抽象的
		 *    AFightingCpuCharacter 子類別。
		 */

#if WITH_EDITORONLY_DATA
		if (const UObject* BlueprintAsset = GetClass()->ClassGeneratedBy)
		{
			SpawnerBlueprintName = BlueprintAsset->GetName();
		}
#endif
		SpawnerBlueprintName.RemoveFromEnd(TEXT("_C"));

		const FString ErrorMessage = FString::Printf(
			TEXT("Blueprint：%s\n屬性設定錯誤：Class Defaults > Cpu Spawner > Cpu Class\n\n請將 Cpu Class 指定為有效且非抽象的 AFightingCpuCharacter 子類別。"),
			*SpawnerBlueprintName);
		UE_LOG(LogTndaFighter, Error, TEXT("%s"), *ErrorMessage);

#if WITH_EDITOR
		if (GEditor && GetWorld()->WorldType == EWorldType::PIE)
		{
			FMessageDialog::Open(
				EAppMsgCategory::Error,
				EAppMsgType::Ok,
				EAppReturnType::Ok,
				FText::FromString(ErrorMessage),
				NSLOCTEXT("FightingCpuSpawner", "InvalidCpuClassTitle", "CPU 生成設定錯誤"));
			GEditor->RequestEndPlayMap();
		}
#endif

		return;
	}

	// 即使出生膠囊與場景碰撞，仍盡量調整位置並保證生成，避免關卡啟動時缺少對手。
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFightingCpuCharacter* SpawnedCpu = GetWorld()->SpawnActor<AFightingCpuCharacter>(CpuClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

	// SpawnActor 仍可能因 World 狀態失敗；後續操作必須以有效結果為前提。
	if (SpawnedCpu)
	{
		// AI Controller 尚未啟用；取得擂台唯一的 1P 出生點來設定初始朝向。
		if (AFightingGameMode* FightingGameMode = GetWorld()->GetAuthGameMode<AFightingGameMode>())
		{
			if (APlayerStart* PlayerStart = FightingGameMode->FindPlayerSpawnPoint())
			{
				FVector DirectionToPlayerStart = PlayerStart->GetActorLocation() - SpawnedCpu->GetActorLocation();
				// CPU 只繞世界 Z 軸轉向，不因雙方高度差產生 Pitch。
				DirectionToPlayerStart.Z = 0.0f;
				if (!DirectionToPlayerStart.IsNearlyZero())
				{
					// 兩點幾乎重合時保留生成朝向，避免用零向量產生不穩定旋轉。
					SpawnedCpu->SetActorRotation(DirectionToPlayerStart.Rotation());
				}
			}
		}

		UE_LOG(LogTndaFighter, Log, TEXT("Spawned cpu: %s"), *SpawnedCpu->GetName());
	}
}

void AFightingCpuSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
