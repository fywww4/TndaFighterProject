// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "Fighting/FightingCameraActor.h"
#include "Fighting/FightingCpuCharacter.h"
#include "Fighting/FightingPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	// SpringArm 必須使用本幀構圖結果，不能先於 Actor 更新而留下上一幀插槽。
	CameraBoom->AddTickPrerequisiteActor(this);

	FightingCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FightingCamera"));
	FightingCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// 固定 FOV，畫面縮放只由 SpringArm 臂長控制，避免同時改變透視感。
	FightingCamera->FieldOfView = 55.0f;
	// stage1 的對戰構圖以 16:9 為基準；其他 Viewport 比例以黑邊保留構圖，避免拉伸。
	FightingCamera->AspectRatio = 16.0f / 9.0f;
	FightingCamera->bConstrainAspectRatio = true;
}

bool AFightingCameraActor::InitializeForFighters(AFightingPlayerCharacter* InPlayer, AFightingCpuCharacter* InCpu)
{
	if (!IsValid(InPlayer) || !IsValid(InCpu))
	{
		return false;
	}
	Player = InPlayer;
	Opponent = InCpu;
	const FVector InitialFocusLocation = (InPlayer->GetActorLocation() + InCpu->GetActorLocation()) * 0.5f
		+ FVector::UpVector * FocusHeight;
	InitialFocusZ = InitialFocusLocation.Z;
	SetActorLocation(InitialFocusLocation);
	CameraBoom->SetRelativeRotation(FixedCameraRotation);
	CameraBoom->SetRelativeRotation(GetDesiredCameraRotation(InPlayer, InCpu));
	CameraBoom->TargetArmLength = GetDesiredCameraDistance(InPlayer, InCpu);

	// 直接設定臂長不會更新插槽；首次同步計算且略過 Lag，避免 View Target 讀到註冊時的舊位置。
	const bool bLocationLag = CameraBoom->bEnableCameraLag;
	const bool bRotationLag = CameraBoom->bEnableCameraRotationLag;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->TickComponent(0.0f, LEVELTICK_All, nullptr);
	CameraBoom->bEnableCameraLag = bLocationLag;
	CameraBoom->bEnableCameraRotationLag = bRotationLag;
	return true;
}

FRotator AFightingCameraActor::GetViewRotation() const
{
	return IsValid(FightingCamera) ? FightingCamera->GetComponentRotation() : GetActorRotation();
}

void AFightingCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AFightingPlayerCharacter* PlayerCharacter = Player.Get();
	AFightingCpuCharacter* CpuCharacter = Opponent.Get();
	// 本次對局只追蹤初始化時指定的角色；失效後停止更新，不搜尋替代對手。
	if (!IsValid(PlayerCharacter) || !IsValid(CpuCharacter))
	{
		return;
	}
	ConstrainPlayerToMaxDistance(PlayerCharacter, CpuCharacter);
	SetActorLocation(GetDesiredFocusLocation(PlayerCharacter, CpuCharacter));
	CameraBoom->SetRelativeRotation(GetDesiredCameraRotation(PlayerCharacter, CpuCharacter));
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, GetDesiredCameraDistance(PlayerCharacter, CpuCharacter), DeltaSeconds, DistanceInterpolationSpeed);
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
