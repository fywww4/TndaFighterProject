// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "Fighting/FightingCameraActor.h"
#include "Fighting/FightingCpuCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TndaFighter.h"

AFightingCameraActor::AFightingCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// 先讓 CharacterMovement 與物理更新完成，再依最終角色位置限制範圍並計算本幀鏡頭。
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	// Actor 本身位於雙方焦點；SpringArm 從這個根節點向後配置實際攝影機位置。
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(CameraRoot);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CameraRoot);
	CameraBoom->TargetArmLength = MinCameraDistance;
	// 格鬥構圖優先於牆面避障，避免 SpringArm 因碰撞自動縮短而讓角色突然放大。
	CameraBoom->bDoCollisionTest = false;

	FightingCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FightingCamera"));
	FightingCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// 固定 FOV，畫面縮放只由 SpringArm 臂長控制，避免同時改變透視感。
	FightingCamera->FieldOfView = 55.0f;
	// stage1 的對戰構圖以 16:9 為基準；其他 Viewport 比例以黑邊保留構圖，避免拉伸。
	FightingCamera->AspectRatio = 16.0f / 9.0f;
	FightingCamera->bConstrainAspectRatio = true;
}

void AFightingCameraActor::InitializeForController(APlayerController* InController)
{
	// 重新初始化前先恢復上一個 Controller 可能留下的隱藏 UI。
	RestoreHiddenWidgetComponents();
	OwningController = InController;
	Opponent.Reset();
	// 這些狀態必須完整重設，否則重新初始化可能沿用上一場的搜尋節流或提早解除遮罩。
	NextOpponentSearchTime = 0.0f;
	bCameraActive = false;
	bPendingFadeRelease = false;

	if (UWorld* World = GetWorld())
	{
		if (ActorSpawnedHandle.IsValid())
		{
			// Initialize 可能被重複呼叫；先移除舊 Handle，避免同一個 CPU 觸發多次回呼。
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}

		// Timer 可能在本 Actor 當幀 Tick 結束後才生成 CPU，因此必須在 Spawn Callback 內立即遮住 UI。
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &AFightingCameraActor::HandleActorSpawned));
	}

	// Camera Fade 只會遮住 3D 場景，不會遮住 HP Bar 這類 Screen-space WidgetComponent。
	if (InController && InController->PlayerCameraManager)
	{
		InController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	}

	// 玩家可能早於 World Spawn Callback 註冊完成前就已存在。
	HideVisibleWidgetComponents(InController ? InController->GetPawn() : nullptr);
}

FRotator AFightingCameraActor::GetViewRotation() const
{
	return IsValid(FightingCamera) ? FightingCamera->GetComponentRotation() : GetActorRotation();
}

void AFightingCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PlayerController = OwningController.Get();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	// Controller 或 Pawn 在關卡切換、死亡重生期間可能暫時失效；等待下一幀重新取得即可。
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	// 等待 CPU 期間持續隱藏玩家的 Screen-space UI。
	if (!bCameraActive)
	{
		HideVisibleWidgetComponents(PlayerPawn);
	}

	if (bPendingFadeRelease)
	{
		// 固定 View Target 已完整渲染一幀，此時才能同時顯示場景與角色 UI。
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
		}

		RestoreHiddenWidgetComponents();
		bPendingFadeRelease = false;
	}

	if (!Opponent.IsValid())
	{
		// 對手死亡或尚未生成時會重新搜尋；RefreshOpponent 內部另有限制查詢頻率。
		RefreshOpponent();
	}

	AFightingCpuCharacter* CurrentOpponent = Opponent.Get();
	if (!IsValid(CurrentOpponent))
	{
		return;
	}

	// 先修正玩家位置，再用修正後座標計算焦點、旋轉與距離，避免畫面落後一幀。
	ConstrainPlayerToMaxDistance(PlayerPawn, CurrentOpponent);

	if (!bCameraActive)
	{
		// 同時處理在 Spawn Callback 註冊前就已存在的對手。
		HideVisibleWidgetComponents(CurrentOpponent);

		const FVector InitialFocusLocation =
			(PlayerPawn->GetActorLocation() + CurrentOpponent->GetActorLocation()) * 0.5f
			+ FVector::UpVector * FocusHeight;
		InitialFocusZ = InitialFocusLocation.Z;

		// 首幀直接套用完整構圖，不做插值，避免從 Actor 預設位置或距離飛入。
		SetActorLocation(InitialFocusLocation);
		CameraBoom->SetRelativeRotation(GetDesiredCameraRotation(PlayerPawn, CurrentOpponent));
		CameraBoom->TargetArmLength = GetDesiredCameraDistance(PlayerPawn, CurrentOpponent);
		PlayerController->SetViewTarget(this);
		if (PlayerController->PlayerCameraManager)
		{
			// 使用 Camera Cut 避免 View Target 插值短暫露出原本的第三人稱視角。
			PlayerController->PlayerCameraManager->SetGameCameraCutThisFrame();
		}

		bCameraActive = true;
		bPendingFadeRelease = true;

		UE_LOG(LogTndaFighter, Log, TEXT("Fighting camera activated for %s and %s."),
			*PlayerPawn->GetName(), *CurrentOpponent->GetName());
		return;
	}

	// 焦點與旋轉不插值，才能讓雙方中點及其畫面水平線在每一幀都準確成立。
	SetActorLocation(GetDesiredFocusLocation(PlayerPawn, CurrentOpponent));
	CameraBoom->SetRelativeRotation(GetDesiredCameraRotation(PlayerPawn, CurrentOpponent));
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, GetDesiredCameraDistance(PlayerPawn, CurrentOpponent), DeltaSeconds, DistanceInterpolationSpeed);
}

void AFightingCameraActor::ConstrainPlayerToMaxDistance(AActor* PlayerActor, const AActor* OpponentActor) const
{
	const FVector PlayerLocation = PlayerActor->GetActorLocation();
	const FVector OpponentLocation = OpponentActor->GetActorLocation();
	FVector OpponentToPlayer = PlayerLocation - OpponentLocation;
	OpponentToPlayer.Z = 0.0f;

	// 邊界只看 XY 平面，跳躍高度不應縮小玩家可用的水平移動範圍。
	const float FighterDistance = OpponentToPlayer.Size();
	if (FighterDistance <= MaxFighterDistance || FighterDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector OutwardDirection = OpponentToPlayer / FighterDistance;
	FVector ConstrainedLocation = OpponentLocation + OutwardDirection * MaxFighterDistance;
	// 只修正水平座標，保留玩家目前的跳躍或落下高度。
	ConstrainedLocation.Z = PlayerLocation.Z;
	// 使用 TeleportPhysics 直接校正位置，避免 Sweep 被場景碰撞擋住而持續留在邊界外。
	PlayerActor->SetActorLocation(ConstrainedLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// 移除向外速度，但保留向內、沿邊界與垂直速度，避免角色持續頂住邊界抖動。
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerActor))
	{
		if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
		{
			const float OutwardSpeed = FVector::DotProduct(MovementComponent->Velocity, OutwardDirection);
			if (OutwardSpeed > 0.0f)
			{
				MovementComponent->Velocity -= OutwardDirection * OutwardSpeed;
			}
		}
	}
}

void AFightingCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 若未主動解除，World Delegate 的生命週期可能長於本 Actor。
	if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		ActorSpawnedHandle.Reset();
	}

	// EndPlay 可能發生在啟動期間，因此銷毀前要解除場景與 UI 兩種遮罩。
	if (APlayerController* PlayerController = OwningController.Get();
		PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
	}

	RestoreHiddenWidgetComponents();
	Super::EndPlay(EndPlayReason);
}

void AFightingCameraActor::HandleActorSpawned(AActor* SpawnedActor)
{
	// UWorld 會在 Actor 完成生成後呼叫此函式，此時 Blueprint 建立的 WidgetComponent 已可取得。
	if (IsValid(Cast<AFightingCpuCharacter>(SpawnedActor)))
	{
		HideVisibleWidgetComponents(SpawnedActor);
	}
}

void AFightingCameraActor::HideVisibleWidgetComponents(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	TInlineComponentArray<UWidgetComponent*> WidgetComponents;
	// TInlineComponentArray 避免為少量常見元件額外配置堆積記憶體。
	Actor->GetComponents(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		// 只記錄原本可見的元件，避免恢復時誤顯示其他遊戲狀態刻意隱藏的 UI。
		if (IsValid(WidgetComponent) && WidgetComponent->IsVisible())
		{
			WidgetComponent->SetVisibility(false);
			HiddenWidgetComponents.Add(WidgetComponent);
		}
	}
}

void AFightingCameraActor::RestoreHiddenWidgetComponents()
{
	for (const TWeakObjectPtr<UWidgetComponent>& WidgetComponent : HiddenWidgetComponents)
	{
		if (WidgetComponent.IsValid())
		{
			WidgetComponent->SetVisibility(true);
		}
	}

	HiddenWidgetComponents.Reset();
}

FVector AFightingCameraActor::GetDesiredFocusLocation(const AActor* PlayerActor, const AActor* OpponentActor) const
{
	// X、Y 永遠取即時中點；只有 Z 需要限制，避免其中一名角色跳躍時鏡頭大幅上下晃動。
	FVector FocusLocation = (PlayerActor->GetActorLocation() + OpponentActor->GetActorLocation()) * 0.5f;
	FocusLocation.Z = FMath::Clamp(
		FocusLocation.Z + FocusHeight,
		InitialFocusZ - MaxVerticalFocusOffset,
		InitialFocusZ + MaxVerticalFocusOffset);
	return FocusLocation;
}

FRotator AFightingCameraActor::GetDesiredCameraRotation(const AActor* PlayerActor, const AActor* OpponentActor) const
{
	FVector FighterAxis = OpponentActor->GetActorLocation() - PlayerActor->GetActorLocation();
	FighterAxis.Z = 0.0f;
	if (FighterAxis.IsNearlyZero())
	{
		// 雙方水平位置重合時沒有可用軸向，保留上一幀旋轉可避免 Yaw 跳到任意值。
		return CameraBoom->GetRelativeRotation();
	}

	// 同一條水平軸有兩個相差 180 度的取景側；選離目前 Yaw 最近的一側可讓角色換邊而不翻鏡。
	const float AxisYaw = FighterAxis.Rotation().Yaw;
	const float FirstSideYaw = AxisYaw - 90.0f;
	const float OtherSideYaw = AxisYaw + 90.0f;
	const float CurrentYaw = CameraBoom->GetRelativeRotation().Yaw;
	const float FirstSideDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, FirstSideYaw));
	const float OtherSideDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, OtherSideYaw));
	const float DesiredYaw = FirstSideDelta <= OtherSideDelta ? FirstSideYaw : OtherSideYaw;

	return FRotator(FixedCameraRotation.Pitch, DesiredYaw, FixedCameraRotation.Roll);
}

float AFightingCameraActor::GetDesiredCameraDistance(const AActor* PlayerActor, const AActor* OpponentActor) const
{
	// Dist2D 忽略跳躍高度，鏡頭遠近只反映角色在格鬥平面上的分離程度。
	const float FighterDistance = FVector::Dist2D(PlayerActor->GetActorLocation(), OpponentActor->GetActorLocation());
	// Max 防止 Blueprint 把上下限設反或設成相同值，避免零長度輸入區間與負向臂長範圍。
	return FMath::GetMappedRangeValueClamped(
		FVector2D(MinFighterDistance, FMath::Max(MinFighterDistance + KINDA_SMALL_NUMBER, MaxFighterDistance)),
		FVector2D(MinCameraDistance, FMath::Max(MinCameraDistance, MaxCameraDistance)),
		FighterDistance);
}

void AFightingCameraActor::RefreshOpponent()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = OwningController.Get();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !IsValid(PlayerPawn) || World->GetTimeSeconds() < NextOpponentSearchTime)
	{
		return;
	}

	// Timer 尚未完成 CPU 生成時，限制搜尋頻率。
	NextOpponentSearchTime = World->GetTimeSeconds() + 0.25f;

	TArray<AActor*> Enemies;
	// 只搜尋 AFightingCpuCharacter 及其 Blueprint 子類，不會誤選場景中的其他 Pawn。
	UGameplayStatics::GetAllActorsOfClass(World, AFightingCpuCharacter::StaticClass(), Enemies);

	// 同時存在多名敵人時選最近者，確保固定攝影機的初始目標一致。
	AFightingCpuCharacter* NearestEnemy = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (AActor* EnemyActor : Enemies)
	{
		AFightingCpuCharacter* Enemy = Cast<AFightingCpuCharacter>(EnemyActor);
		if (!IsValid(Enemy))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), Enemy->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestEnemy = Enemy;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	Opponent = NearestEnemy;
	// 沒有有效候選時保存空弱參照，下一個節流週期會繼續尋找。
}
