#include "EnemyHitReaction.h"
#include "Enemy.h"
#include "Player.h"
#include "EnemyAttackManager.h"
#include <cmath>

void EnemyHitReaction::CheckPlayerRushStatus(Player* player)
{
	if (player == nullptr) { return; }

	bool isRushActive = player->IsRushActive();

	if (wasRushActive_ && !isRushActive) {
		if (!rushFinalHitReceived_) {
			isBeingRushed_ = false;
		}
		rushFinalHitReceived_ = false;
	}

	wasRushActive_ = isRushActive;
}

void EnemyHitReaction::UpdateStun()
{
	rushStunTimer_--;
	if (rushStunTimer_ <= 0) {
		isRushStunned_ = false;
		rushStunTimer_ = 0;
	}
}

void EnemyHitReaction::OnRushHit(bool isFinalHit, Enemy* enemy)
{
	if (!enemy->GetIsAlive()) { return; }

	// 被弾時の揺れを開始
	OnHit();

	if (isFinalHit) {
		isRushStunned_ = true;
		rushStunTimer_ = kRushStunDuration_ * kFinalHitStunMultiplier_;
		rushFinalHitReceived_ = false;
		isBeingRushed_ = false;
	}
	else {
		isRushStunned_ = true;
		rushStunTimer_ = kRushStunDuration_;
	}

	// 攻撃を中断させる（全ヒットで中断させる）
	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
	}
}

void EnemyHitReaction::OnHit()
{
	isWobbling_ = true;
	wobbleTimer_ = kWobbleDuration_;
	// Phase はリセットせず継続させることで、連続ヒット時も動きがつながるようにする
}

void EnemyHitReaction::UpdateWobble()
{
	if (!isWobbling_) {
		wobblePhase_ = 0.0f;
		return;
	}

	wobbleTimer_--;
	if (wobbleTimer_ <= 0) {
		isWobbling_ = false;
		wobbleTimer_ = 0;
		wobbleRotation_ = { 0.0f, 0.0f, 0.0f };
		wobblePhase_ = 0.0f;
		return;
	}

	wobblePhase_ += kWobbleFrequency_;

	float t = static_cast<float>(wobbleTimer_) / static_cast<float>(kWobbleDuration_);
	float decay = t * t; // 二乗で減衰

	// サイン波で揺らす。Phase を使うことで連続ヒット時も不自然なジャンプを防ぐ
	wobbleRotation_.z = sin(wobblePhase_) * kWobbleMaxAngle_ * decay;
	wobbleRotation_.x = cos(wobblePhase_ * 0.7f) * kWobbleMaxAngle_ * 0.5f * decay;
}

void EnemyHitReaction::StartKnockback(Enemy* enemy, Player* player)
{
	isBeingRushed_ = true;
	rushKnockbackTimer_ = 0;
	knockbackSpeed_ = initialKnockbackSpeed_;
	knockbackVerticalVelocity_ = knockbackInitialVerticalVelocity_;
	knockbackGroundY_ = enemy->GetCenterPosition().y;

	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
	}

	Vector3 originalRot = enemy->GetOriginalRotation();
	Vector3 currentRot = enemy->GetWorldRotation();
	enemy->SetObjRotation(Vector3(originalRot.x, currentRot.y, originalRot.z));

	if (player != nullptr) {
		Vector3 direction = enemy->GetCenterPosition() - player->GetCenterPosition();
		direction.y = 0.0f;
		knockbackDirection_ = (direction.Length() > kKnockbackDirectionMinLength_)
			? direction.Normalize()
			: Vector3(0.0f, 0.0f, 1.0f);
	}
}

void EnemyHitReaction::UpdateKnockback(Enemy* enemy)
{
	rushKnockbackTimer_++;

	if (rushKnockbackTimer_ >= kMaxRushKnockbackDuration_) {
		EndKnockback(enemy);
		return;
	}

	Vector3 currentPos = enemy->GetCenterPosition();

	// 縦方向（ノックアップ）
	knockbackVerticalVelocity_ -= knockbackGravity_;
	float newY = currentPos.y + knockbackVerticalVelocity_;

	if (newY <= knockbackGroundY_ && knockbackVerticalVelocity_ <= 0.0f) {
		newY = knockbackGroundY_;
		knockbackVerticalVelocity_ = 0.0f;
	}

	// 水平方向（吹っ飛び）
	Vector3 horizontalVelocity = knockbackDirection_ * knockbackSpeed_;
	knockbackSpeed_ *= knockbackDecay_;
	if (knockbackSpeed_ < 0.001f) { knockbackSpeed_ = 0.0f; }

	Vector3 newPos = currentPos + horizontalVelocity;
	newPos.y = newY;
	enemy->SetWorldPosition(newPos);

	// 回転演出
	Vector3 currentRot = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();
	float   tiltAmount = 0.0f;

	bool isAirborne = (newY > knockbackGroundY_ + kAirborneThreshold_);
	if (isAirborne) {
		tiltAmount = -knockbackVerticalVelocity_ * (maxTiltAngle_ / knockbackInitialVerticalVelocity_);
	}
	else {
		float decayFactor = 1.0f - static_cast<float>(rushKnockbackTimer_) / static_cast<float>(kMaxRushKnockbackDuration_);
		tiltAmount = sin(static_cast<float>(rushKnockbackTimer_) * kGroundTiltFrequency_) * maxTiltAngle_ * decayFactor * kGroundTiltDampingScale_;
	}

	Vector3 objRot = currentRot;
	objRot.x = originalRot.x + tiltAmount;
	enemy->SetObjRotation(objRot);
}

void EnemyHitReaction::EndKnockback(Enemy* enemy)
{
	isBeingRushed_ = false;
	rushKnockbackTimer_ = 0;
	knockbackSpeed_ = initialKnockbackSpeed_;
	knockbackVerticalVelocity_ = 0.0f;

	Vector3 pos = enemy->GetCenterPosition();
	if (pos.y < knockbackGroundY_) {
		pos.y = knockbackGroundY_;
		enemy->SetWorldPosition(pos);
	}

	Vector3 currentRot = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();
	enemy->SetObjRotation(Vector3(originalRot.x, currentRot.y, originalRot.z));
}