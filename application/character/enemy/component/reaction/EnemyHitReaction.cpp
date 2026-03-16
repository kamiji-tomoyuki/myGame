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

	if (isFinalHit) {
		isRushStunned_        = true;
		rushStunTimer_        = kRushStunDuration_ * 3;
		rushFinalHitReceived_ = false;
		isBeingRushed_        = false;

		if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
			mgr->InterruptByRush();
		}
	}
	else {
		isRushStunned_ = true;
		rushStunTimer_ = kRushStunDuration_;
	}
}

void EnemyHitReaction::StartKnockback(Enemy* enemy, Player* player)
{
	isBeingRushed_              = true;
	rushKnockbackTimer_         = 0;
	knockbackSpeed_             = initialKnockbackSpeed_;
	knockbackVerticalVelocity_  = knockbackInitialVerticalVelocity_;
	knockbackGroundY_           = enemy->GetCenterPosition().y;

	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush();
	}

	Vector3 originalRot = enemy->GetOriginalRotation();
	Vector3 currentRot  = enemy->GetWorldRotation();
	enemy->SetObjRotation(Vector3(originalRot.x, currentRot.y, originalRot.z));

	if (player != nullptr) {
		Vector3 direction = enemy->GetCenterPosition() - player->GetCenterPosition();
		direction.y = 0.0f;
		knockbackDirection_ = (direction.Length() > 0.001f)
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
	Vector3 currentRot  = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();
	float   tiltAmount  = 0.0f;

	bool isAirborne = (newY > knockbackGroundY_ + 0.01f);
	if (isAirborne) {
		tiltAmount = -knockbackVerticalVelocity_ * (maxTiltAngle_ / knockbackInitialVerticalVelocity_);
	}
	else {
		float decayFactor = 1.0f - static_cast<float>(rushKnockbackTimer_) / static_cast<float>(kMaxRushKnockbackDuration_);
		tiltAmount = sin(static_cast<float>(rushKnockbackTimer_) * 0.4f) * maxTiltAngle_ * decayFactor * 0.5f;
	}

	Vector3 objRot = currentRot;
	objRot.x = originalRot.x + tiltAmount;
	enemy->SetObjRotation(objRot);
}

void EnemyHitReaction::EndKnockback(Enemy* enemy)
{
	isBeingRushed_             = false;
	rushKnockbackTimer_        = 0;
	knockbackSpeed_            = initialKnockbackSpeed_;
	knockbackVerticalVelocity_ = 0.0f;

	Vector3 pos = enemy->GetCenterPosition();
	if (pos.y < knockbackGroundY_) {
		pos.y = knockbackGroundY_;
		enemy->SetWorldPosition(pos);
	}

	Vector3 currentRot  = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();
	enemy->SetObjRotation(Vector3(originalRot.x, currentRot.y, originalRot.z));
}
