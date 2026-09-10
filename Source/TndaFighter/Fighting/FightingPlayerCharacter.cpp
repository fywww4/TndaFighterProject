// Copyright © 2026 USERJOY Technology Co., Ltd.All Rights Reserved.

#include "FightingPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "FightingPlayerController.h"

AFightingPlayerCharacter::AFightingPlayerCharacter()
{
	// 與目前 Manny 角色尺寸一致，讓碰撞中心和角色腳底位置保持既有配置。
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// 原生預設步行速度；Blueprint 子類仍可在 Class Defaults 覆寫。
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;

	// 供使用 Actor Tag 查詢的執行期系統辨識這名玩家角色。
	Tags.Add(FName("Player"));
}

void AFightingPlayerCharacter::Move(const FInputActionValue& Value)
{
	// MoveAction 約定輸出 Vector2D：X 是左右，Y 是前後。
	FVector2D MovementVector = Value.Get<FVector2D>();

	// 集中走 DoMove，讓 Enhanced Input 與 Blueprint／UI 共用完全相同的方向換算。
	DoMove(MovementVector.X, MovementVector.Y);
}

void AFightingPlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// 共用攝影機尚未取得兩名角色時，先沿用 Controller 旋轉作為移動基準。
		FRotator Rotation = GetController()->GetControlRotation();
		if (const AFightingPlayerController* PlayerController = Cast<AFightingPlayerController>(GetController()))
		{
			// 共用攝影機啟用後，以實際畫面方向作為相對攝影機移動的唯一基準。
			PlayerController->GetFightingCameraRotation(Rotation);
		}
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 只取 Yaw，避免攝影機俯角讓水平移動產生垂直分量。
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// 右方向和前方向使用同一份 Yaw 基準，確保搖桿兩軸在畫面空間互相垂直。
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// CharacterMovement 會合成兩軸輸入並限制最大加速度，不需在這裡自行正規化。
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AFightingPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFightingPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AFightingPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 只有 Enhanced Input Component 才能綁定 Input Action；其他元件類型交由父類處理。
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 使用 Triggered 讓按住輸入時每幀持續更新移動向量。
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFightingPlayerCharacter::Move);
	}
}
