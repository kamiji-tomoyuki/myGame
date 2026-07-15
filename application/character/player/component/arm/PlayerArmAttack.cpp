#include "PlayerArmAttack.h"
#include <cmath>

using namespace Engine;
const std::string PlayerArmAttack::kGroupName_ = "PlayerArmAttack";

// =============================================================
//  コンストラクタ — GlobalVariables への登録
// =============================================================
PlayerArmAttack::PlayerArmAttack()
{
	variables_ = GlobalVariables::GetInstance();

	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	// 攻撃モーション
	variables_->AddItem(kGroupName_, "Attack Duration", static_cast<int32_t>(attackDuration_));
	variables_->AddItem(kGroupName_, "Combo Window", static_cast<int32_t>(comboWindow_));
	// 腕移動量
	variables_->AddItem(kGroupName_, "Attack Distance", attackDistance_);
	variables_->AddItem(kGroupName_, "Right Punch Offset X", rightPunchOffsetX_);
	variables_->AddItem(kGroupName_, "Left Punch Offset X", leftPunchOffsetX_);
	// ダメージ
	variables_->AddItem(kGroupName_, "Attack Damage", static_cast<int32_t>(attackDamage_));
}

// =============================================================
//  攻撃開始
// =============================================================
void PlayerArmAttack::StartAttack(AttackType attackType, bool isRightArm, const Vector3& currentTranslation)
{
	ApplyVariables();

	isAttack_ = true;
	currentAttackType_ = attackType;
	attackTimer_ = 0;
	attackProgress_ = 0.0f;
	hasHitThisAttack_ = false;	// ヒットフラグをリセット

	if (attackType == AttackType::kRightPunch) {
		comboTimer_ = comboWindow_;
		comboCount_ = 1;
	}
	else if (attackType == AttackType::kLeftPunch) {
		comboTimer_ = comboWindow_;
		comboCount_ = 2;
	}

	Vector3 attackOffset = { 0.0f, 0.0f, attackDistance_ };

	switch (attackType) {
	case AttackType::kRightPunch:
		attackOffset.x += rightPunchOffsetX_;
		break;
	case AttackType::kLeftPunch:
		attackOffset.x += leftPunchOffsetX_;
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
//  更新
// =============================================================
bool PlayerArmAttack::Update()
{
	if (!isAttack_) { return false; }

	attackTimer_++;
	attackProgress_ = static_cast<float>(attackTimer_) / static_cast<float>(attackDuration_);
	if (attackProgress_ >= 1.0f) { attackProgress_ = 1.0f; }

	float easedProgress = 1.0f - (1.0f - attackProgress_) * (1.0f - attackProgress_);
	if (attackProgress_ > kEasingTurnPoint_) {
		easedProgress = 1.0f - (attackProgress_ - kEasingTurnPoint_) * 2.0f;
	}

	currentTranslation_ = {
		originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
		originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
		originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
	};

	if (attackTimer_ >= attackDuration_) {
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
//  UpdateComboTimer
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

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void PlayerArmAttack::ApplyVariables()
{
	attackDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Attack Duration"));
	comboWindow_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Combo Window"));
	attackDistance_ = variables_->GetFloatValue(kGroupName_, "Attack Distance");
	rightPunchOffsetX_ = variables_->GetFloatValue(kGroupName_, "Right Punch Offset X");
	leftPunchOffsetX_ = variables_->GetFloatValue(kGroupName_, "Left Punch Offset X");
	attackDamage_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Attack Damage"));
}