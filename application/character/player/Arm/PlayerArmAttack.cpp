#include "PlayerArmAttack.h"
#include <cmath>

// =============================================================
//  攻撃開始
// =============================================================
void PlayerArmAttack::StartAttack(AttackType attackType, bool isRightArm, const Vector3& currentTranslation)
{
	isAttack_ = true;
	currentAttackType_ = attackType;
	attackTimer_ = 0;
	attackProgress_ = 0.0f;
	hasHitThisAttack_ = false;	// ヒットフラグをリセット

	if (attackType == AttackType::kRightPunch) {
		comboTimer_ = kComboWindow;
		comboCount_ = 1;
	}
	else if (attackType == AttackType::kLeftPunch) {
		comboTimer_ = kComboWindow;
		comboCount_ = 2;
	}

	Vector3 attackOffset = { 0.0f, 0.0f, kAttackDistance };

	switch (attackType) {
	case AttackType::kRightPunch:
		attackOffset.x += -0.3f;
		break;
	case AttackType::kLeftPunch:
		attackOffset.x += 0.3f;
		break;
	default:
		break;
	}

	originalPosition_ = currentTranslation;
	currentTranslation_ = currentTranslation;
	targetPosition_ = {
		originalPosition_.x + attackOffset.x,
		originalPosition_.y + attackOffset.y,
		originalPosition_.z + attackOffset.z
	};

	attackDirection_ = attackOffset;
	float len = sqrt(attackDirection_.x * attackDirection_.x +
		attackDirection_.y * attackDirection_.y +
		attackDirection_.z * attackDirection_.z);
	if (len > 0.0f) {
		attackDirection_.x /= len;
		attackDirection_.y /= len;
		attackDirection_.z /= len;
	}
}

// =============================================================
//  更新（毎フレーム）
//  戻り値: 攻撃終了なら true
// =============================================================
bool PlayerArmAttack::Update()
{
	if (!isAttack_) { return false; }

	attackTimer_++;
	attackProgress_ = static_cast<float>(attackTimer_) / static_cast<float>(kAttackDuration);
	if (attackProgress_ >= 1.0f) { attackProgress_ = 1.0f; }

	float easedProgress = 1.0f - (1.0f - attackProgress_) * (1.0f - attackProgress_);
	if (attackProgress_ > 0.5f) {
		easedProgress = 1.0f - (attackProgress_ - 0.5f) * 2.0f;
	}

	currentTranslation_ = {
		originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
		originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
		originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
	};

	if (attackTimer_ >= kAttackDuration) {
		lastAttackType_ = currentAttackType_;
		currentAttackType_ = AttackType::kNone;
		isAttack_ = false;
		attackTimer_ = 0;
		attackProgress_ = 0.0f;
		hasHitThisAttack_ = false;	// 次の攻撃に備えてリセット
		currentTranslation_ = originalPosition_;
		return true;	// 攻撃終了
	}

	return false;
}

// =============================================================
//  コンボウィンドウ更新
// =============================================================
void PlayerArmAttack::UpdateComboTimer()
{
	if (comboTimer_ > 0) {
		comboTimer_--;
		if (comboTimer_ == 0) {
			comboCount_ = 0;
		}
	}
}