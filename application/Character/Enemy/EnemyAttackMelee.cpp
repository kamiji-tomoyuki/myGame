#include "EnemyAttackMelee.h"
#include "Enemy.h"
#include "Player.h"
#include <ParticleEmitter.h>
#include <cmath>

EnemyAttackMelee::EnemyAttackMelee()
{
	trailEffect_ = std::make_unique<ParticleEmitter>();
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

	// 軌跡エフェクトの初期化
	trailEffect_->Initialize("enemyTrail", "debug/plane.obj");
}

void EnemyAttackMelee::Start(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	preparationTimer_ = 0;
	chargingTimer_ = 0;
	recoveryTimer_ = 0;
	chargeCount_ = 0;

	// 元の回転を保存
	originalRotation_ = enemy->GetCenterRotation();

	// プレイヤーへの方向を計算
	Vector3 targetPos = player->GetCenterPosition();
	Vector3 currentPos = enemy->GetCenterPosition();
	chargeDirection_ = (targetPos - currentPos).Normalize();
	chargeStartPos_ = currentPos;

	// プレイヤーの方向を向く
	float targetRotationY = std::atan2(chargeDirection_.x, chargeDirection_.z);
	enemy->SetRotationY(targetRotationY);

	// 軌跡エフェクトの初期位置設定
	Vector3 initialFootPos = enemy->GetWorldPosition();
	initialFootPos.y += kFootOffsetY_;
	lastTrailPosition_ = initialFootPos;
}

void EnemyAttackMelee::Update(Enemy* enemy, Player* player)
{
	if (enemy == nullptr) return;

	switch (phase_) {
	case Phase::kPreparation:
		UpdatePreparation(enemy);
		break;

	case Phase::kCharging:
		UpdateCharging(enemy);
		break;

	case Phase::kRecovery:
		UpdateRecovery(enemy);
		break;
	}
}

void EnemyAttackMelee::UpdatePreparation(Enemy* enemy)
{
	preparationTimer_++;

	// 硬直状態を維持(移動しない)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// 前に傾く予備動作アニメーション
	if (preparationTimer_ < kPreparationTime_) {
		float preparationProgress = static_cast<float>(preparationTimer_) / kPreparationTime_;
		float tiltAmount = preparationProgress * kPreparationTiltAngle_;

		Vector3 baseRotation = enemy->GetWorldRotation();
		Vector3 objRotation = baseRotation;
		objRotation.x = originalRotation_.x + tiltAmount;
		enemy->SetRotation(objRotation);
	}

	// 予備動作完了後、突進開始
	if (preparationTimer_ >= kPreparationTime_) {
		phase_ = Phase::kCharging;
		chargingTimer_ = 0;
		chargeCount_++;
	}
}

void EnemyAttackMelee::UpdateCharging(Enemy* enemy)
{
	chargingTimer_++;

	// 突進移動
	Vector3 chargeVelocity = chargeDirection_ * kChargeSpeed_;
	enemy->SetWorldPosition(enemy->GetCenterPosition() + chargeVelocity);

	// 軌跡エフェクトの更新
	UpdateTrailEffect(enemy);

	// 突進時間が終了したら回復フェーズへ
	if (chargingTimer_ >= kChargingTime_) {
		phase_ = Phase::kRecovery;
		recoveryTimer_ = 0;
	}
}

void EnemyAttackMelee::UpdateRecovery(Enemy* enemy)
{
	recoveryTimer_++;

	// 硬直状態を維持(移動しない)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// 傾きを徐々に元に戻す補間処理
	float recoveryProgress = static_cast<float>(recoveryTimer_) / kRecoveryTime_;
	float currentTilt = kPreparationTiltAngle_ * (1.0f - recoveryProgress);

	Vector3 baseRotation = enemy->GetWorldRotation();
	Vector3 objRotation = baseRotation;
	objRotation.x = originalRotation_.x + currentTilt;
	enemy->SetRotation(objRotation);

	// 回復時間が終了したら次の行動へ
	if (recoveryTimer_ >= kRecoveryTime_) {
		// 完全に元の姿勢に戻す
		Vector3 currentRotation = enemy->GetWorldRotation();
		currentRotation.x = originalRotation_.x;
		enemy->SetRotation(currentRotation);

		// まだ突進回数が残っている場合
		if (chargeCount_ < maxChargeCount_) {
			// 次の突進の準備
			phase_ = Phase::kPreparation;
			preparationTimer_ = kPreparationTime_ - kNextChargeDelay_;
		}
		else {
			// 全ての突進が終了
			phase_ = Phase::kNone;
			isComplete_ = true;
		}
	}
}

void EnemyAttackMelee::Interrupt()
{
	phase_ = Phase::kNone;
	isComplete_ = true;
	chargeCount_ = 0;
}

void EnemyAttackMelee::UpdateTrailEffect(Enemy* enemy)
{
	if (enemy == nullptr) return;

	Vector3 currentPos = enemy->GetCenterPosition();

	// 足元の位置を計算
	Vector3 footPosition = currentPos;
	footPosition.y += kFootOffsetY_;

	float distanceMoved = (footPosition - lastTrailPosition_).Length();

	// 一定距離移動したらパーティクルを発生
	if (distanceMoved >= trailEmitDistance_) {
		// 移動している場合のみパーティクルを発生
		Vector3 velocity = enemy->GetVelocity();
		if (velocity.Length() > 0.01f) {
			trailEffect_->SetPosition(footPosition);
			trailEffect_->SetActive(false);
		}
		lastTrailPosition_ = footPosition;
	}
}

void EnemyAttackMelee::DrawTrailEffect()
{
	trailEffect_->Draw(Normal);
}