#include "EnemyAttackMelee.h"
#include "Enemy.h"
#include "Player.h"
#include "StageManager.h"
#include <ParticleEmitter.h>
#include <cmath>

const std::string EnemyAttackMelee::kGroupName_ = "EnemyAttackMelee";

EnemyAttackMelee::EnemyAttackMelee()
{
	variables_ = GlobalVariables::GetInstance();

	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	// タイマー系
	variables_->AddItem(kGroupName_, "Preparation Time", static_cast<int32_t>(kPreparationTime_));
	variables_->AddItem(kGroupName_, "Charging Time", static_cast<int32_t>(kChargingTime_));
	variables_->AddItem(kGroupName_, "Recovery Time", static_cast<int32_t>(kRecoveryTime_));
	variables_->AddItem(kGroupName_, "Next Charge Delay", static_cast<int32_t>(kNextChargeDelay_));
	// 挙動
	variables_->AddItem(kGroupName_, "Charge Speed", kChargeSpeed_);
	variables_->AddItem(kGroupName_, "Preparation Tilt Angle", kPreparationTiltAngle_);
	variables_->AddItem(kGroupName_, "Melee Hit Radius", kMeleeHitRadius_);
	variables_->AddItem(kGroupName_, "Melee Damage", kMeleeDamage_);
}

EnemyAttackMelee::~EnemyAttackMelee() = default;

void EnemyAttackMelee::Initialize()
{
	phase_ = Phase::kNone;
	isComplete_ = false;
	chargeCount_ = 0;
	maxChargeCount_ = 1;
	preparationTimer_ = 0;
	chargingTimer_ = 0;
	recoveryTimer_ = 0;
}

void EnemyAttackMelee::Start(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;

	ApplyVariables();

	if (enemy->GetIsPhase2()) {
		maxChargeCount_ = 2;
	}
	else {
		maxChargeCount_ = 1;
	}

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	preparationTimer_ = 0;
	chargingTimer_ = 0;
	recoveryTimer_ = 0;
	chargeCount_ = 0;
	hitRegistered_ = false;

	// 元の回転を保存
	originalRotation_ = enemy->GetCenterRotation();

	// プレイヤーへの方向を計算
	Vector3 targetPos = player->GetCenterPosition();
	Vector3 currentPos = enemy->GetCenterPosition();
	chargeDirection_ = (targetPos - currentPos).Normalize();
	chargeStartPos_ = currentPos;
	groundY_ = currentPos.y;  // 突進中のY座標を地面に固定するため記録

	// プレイヤーの方向を向く
	float targetRotationY = std::atan2(chargeDirection_.x, chargeDirection_.z);
	enemy->SetRotationY(targetRotationY);
}

void EnemyAttackMelee::Update(Enemy* enemy, Player* player)
{
	if (enemy == nullptr) return;

	switch (phase_) {
	case Phase::kPreparation:
		UpdatePreparation(enemy);
		break;

	case Phase::kCharging:
		UpdateCharging(enemy, player);
		break;

	case Phase::kRecovery:
		UpdateRecovery(enemy);
		break;
	}
}

void EnemyAttackMelee::UpdatePreparation(Enemy* enemy)
{
	preparationTimer_++;

	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// プレイヤーを追跡（Y軸旋回）
	Player* player = enemy->GetPlayer();
	if (player) {
		Vector3 targetPos = player->GetCenterPosition();
		Vector3 currentPos = enemy->GetCenterPosition();
		Vector3 toPlayer = (targetPos - currentPos).Normalize();
		float targetRotationY = std::atan2(toPlayer.x, toPlayer.z);

		float currentRotY = enemy->GetWorldRotation().y;
		float angleDiff = targetRotationY - currentRotY;

		// 最短角計算
		while (angleDiff > 3.14159265f) { angleDiff -= 6.2831853f; }
		while (angleDiff < -3.14159265f) { angleDiff += 6.2831853f; }

		// 旋回スピード
		float newRotY = currentRotY + angleDiff * 0.15f;
		enemy->SetRotationY(newRotY);

		// 突進開始座標と方向を常に最新に更新
		chargeDirection_ = { std::sin(newRotY), 0.0f, std::cos(newRotY) };
		chargeStartPos_ = currentPos;
	}

	if (preparationTimer_ < kPreparationTime_) {
		float preparationProgress = static_cast<float>(preparationTimer_) / static_cast<float>(kPreparationTime_);
		float tiltAmount = preparationProgress * kPreparationTiltAngle_;

		Vector3 baseRotation = enemy->GetWorldRotation();
		Vector3 objRotation = baseRotation;
		objRotation.x = originalRotation_.x + tiltAmount;
		enemy->SetRotation(objRotation);
	}

	if (preparationTimer_ >= kPreparationTime_) {
		phase_ = Phase::kCharging;
		chargingTimer_ = 0;
		chargeCount_++;
	}
}

void EnemyAttackMelee::UpdateCharging(Enemy* enemy, Player* player)
{
	chargingTimer_++;

	Vector3 chargeVelocity = chargeDirection_ * kChargeSpeed_;
	chargeVelocity.y = 0.0f;  // 突進はXZ平面のみ
	Vector3 newPos = enemy->GetCenterPosition() + chargeVelocity;
	newPos.y = groundY_;       // 地面Y座標に固定

	// ステージ境界内に制限
	newPos = StageManager::GetInstance()->ClampToStageBounds(newPos);

	enemy->SetWorldPosition(newPos);

	CheckCollision(player);

	if (chargingTimer_ >= kChargingTime_) {
		phase_ = Phase::kRecovery;
		recoveryTimer_ = 0;
	}
}

void EnemyAttackMelee::UpdateRecovery(Enemy* enemy)
{
	recoveryTimer_++;

	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	float recoveryProgress = static_cast<float>(recoveryTimer_) / static_cast<float>(kRecoveryTime_);
	float currentTilt = kPreparationTiltAngle_ * (1.0f - recoveryProgress);

	Vector3 baseRotation = enemy->GetWorldRotation();
	Vector3 objRotation = baseRotation;
	objRotation.x = originalRotation_.x + currentTilt;
	enemy->SetRotation(objRotation);

	if (recoveryTimer_ >= kRecoveryTime_) {
		Vector3 currentRotation = enemy->GetWorldRotation();
		currentRotation.x = originalRotation_.x;
		enemy->SetRotation(currentRotation);

		if (chargeCount_ < maxChargeCount_) {
			phase_ = Phase::kPreparation;
			preparationTimer_ = kPreparationTime_ - kNextChargeDelay_;
			hitRegistered_ = false;

			// 座標だけ最新にしておく。向きの旋回は UpdatePreparation に委譲する
			chargeStartPos_ = enemy->GetCenterPosition();
		}
		else {
			phase_ = Phase::kNone;
			isComplete_ = true;
		}
	}
}

void EnemyAttackMelee::CheckCollision(Player* player)
{
	if (player == nullptr) return;
	if (hitRegistered_) return;

	Vector3 enemyPos = chargeStartPos_ + chargeDirection_ * (kChargeSpeed_ * static_cast<float>(chargingTimer_));
	Vector3 playerPos = player->GetCenterPosition();

	float distanceXZ = std::sqrt(
		std::pow(playerPos.x - enemyPos.x, 2.0f) +
		std::pow(playerPos.z - enemyPos.z, 2.0f)
	);

	if (distanceXZ <= kMeleeHitRadius_) {
		if (!player->IsDodging()) {
			player->ApplyDamage(static_cast<uint32_t>(kMeleeDamage_), enemyPos);
			hitRegistered_ = true;
		}
	}
}

void EnemyAttackMelee::Interrupt(Enemy* enemy)
{
	// 攻撃中断時に回転をリセット
	if (enemy) {
		Vector3 originalRot = enemy->GetOriginalRotation();
		Vector3 rot = enemy->GetWorldRotation();
		rot.x = originalRot.x;
		enemy->SetRotation(rot);
	}
	phase_ = Phase::kNone;
	isComplete_ = true;
	chargeCount_ = 0;
}

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void EnemyAttackMelee::ApplyVariables()
{
	kPreparationTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Preparation Time"));
	kChargingTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Charging Time"));
	kRecoveryTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recovery Time"));
	kNextChargeDelay_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Next Charge Delay"));
	kChargeSpeed_ = variables_->GetFloatValue(kGroupName_, "Charge Speed");
	kPreparationTiltAngle_ = variables_->GetFloatValue(kGroupName_, "Preparation Tilt Angle");
	kMeleeHitRadius_ = variables_->GetFloatValue(kGroupName_, "Melee Hit Radius");
	kMeleeDamage_ = variables_->GetIntValue(kGroupName_, "Melee Damage");
}