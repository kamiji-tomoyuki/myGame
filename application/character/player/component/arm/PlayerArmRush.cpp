#include "PlayerArmRush.h"
#include <cmath>

const std::string PlayerArmRush::kGroupName_ = "PlayerArmRush";

// =============================================================
//  コンストラクタ — GlobalVariables への登録
// =============================================================
PlayerArmRush::PlayerArmRush()
{
	variables_ = GlobalVariables::GetInstance();

	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	// 連打フェーズ
	variables_->AddItem(kGroupName_, "Rush Duration", static_cast<int32_t>(kRushDuration_));
	variables_->AddItem(kGroupName_, "Rush Interval", static_cast<int32_t>(kRushInterval_));
	variables_->AddItem(kGroupName_, "Rush Attack Duration", static_cast<int32_t>(kRushAttackDuration_));
	// フィニッシャーフェーズ
	variables_->AddItem(kGroupName_, "WindUp Duration", static_cast<int32_t>(kWindUpDuration_));
	variables_->AddItem(kGroupName_, "Finisher Duration", static_cast<int32_t>(kFinisherDuration_));
	variables_->AddItem(kGroupName_, "Recover Duration", static_cast<int32_t>(kRecoverDuration_));
	// 腕移動量
	variables_->AddItem(kGroupName_, "Rush Distance", kRushDistance_);
	variables_->AddItem(kGroupName_, "WindUp Arm Retreat", kWindUpArmRetreat_);
	variables_->AddItem(kGroupName_, "WindUp Arm Side R", kWindUpArmSideR_);
	variables_->AddItem(kGroupName_, "Finisher Arm Advance", kFinisherArmAdvance_);
	variables_->AddItem(kGroupName_, "L Arm WindUp Z", kLArmWindUpZ_);
	variables_->AddItem(kGroupName_, "L Arm Finisher Z", kLArmFinisherZ_);
	// ダメージ
	variables_->AddItem(kGroupName_, "Rush Attack Damage", static_cast<int32_t>(rushAttackDamage_));
	variables_->AddItem(kGroupName_, "Finisher Attack Damage", static_cast<int32_t>(finisherAttackDamage_));
}

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void PlayerArmRush::ApplyVariables()
{
	kRushDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Rush Duration"));
	kRushInterval_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Rush Interval"));
	kRushAttackDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Rush Attack Duration"));
	kWindUpDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "WindUp Duration"));
	kFinisherDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Finisher Duration"));
	kRecoverDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recover Duration"));
	kRushDistance_ = variables_->GetFloatValue(kGroupName_, "Rush Distance");
	kWindUpArmRetreat_ = variables_->GetFloatValue(kGroupName_, "WindUp Arm Retreat");
	kWindUpArmSideR_ = variables_->GetFloatValue(kGroupName_, "WindUp Arm Side R");
	kFinisherArmAdvance_ = variables_->GetFloatValue(kGroupName_, "Finisher Arm Advance");
	kLArmWindUpZ_ = variables_->GetFloatValue(kGroupName_, "L Arm WindUp Z");
	kLArmFinisherZ_ = variables_->GetFloatValue(kGroupName_, "L Arm Finisher Z");
	rushAttackDamage_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Rush Attack Damage"));
	finisherAttackDamage_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Finisher Attack Damage"));
}

// =============================================================
//  ラッシュ開始
// =============================================================
void PlayerArmRush::StartRush(bool isRightArm, const Vector3& currentTranslation)
{
	ApplyVariables();

	isRush_ = true;
	isRightArm_ = isRightArm;
	rushPhase_ = RushPhase::kRapidPunch;
	rushTimer_ = 0;
	rushAttackTimer_ = 0;
	rushCount_ = 0;
	rushAttackActive_ = false;
	rushPhaseTimer_ = 0;
	rushPhaseProgress_ = 0.0f;
	finisherProgress_ = 0.0f;
	isFinisherHitFrame_ = false;
	hasFinisherHit_ = false;
	lastRushHitFrame_ = -999;
	bodyRotYAtWindUpStart_ = 0.0f;
	currentBodyRotY_ = 0.0f;

	originalPosition_ = currentTranslation;
	currentTranslation_ = currentTranslation;
}

bool PlayerArmRush::Update(float currentBodyRotY)
{
	if (!isRush_) { return false; }

	currentBodyRotY_ = currentBodyRotY;
	rushPhaseTimer_++;

	switch (rushPhase_) {
	case RushPhase::kRapidPunch: UpdateRapidPunch(); break;
	case RushPhase::kWindUp:     UpdateWindUp();     break;
	case RushPhase::kFinisher:   UpdateFinisher();   break;
	case RushPhase::kRecover:
		UpdateRecover();
		if (!isRush_) { return true; }	// Recover終了でラッシュ全体終了
		break;
	}

	return false;
}

Vector3 PlayerArmRush::WorldOffsetToLocal(const Vector3& worldOffset, float currentBodyRotY) const
{
	// WindUp開始時からの体の回転差分（ひねり量）
	float deltaRot = currentBodyRotY - bodyRotYAtWindUpStart_;

	// ローカル座標に戻すには逆回転（-deltaRot）を掛ける
	float cosA = cosf(-deltaRot);
	float sinA = sinf(-deltaRot);

	Vector3 localOffset;
	localOffset.x = worldOffset.x * cosA - worldOffset.z * sinA;
	localOffset.y = worldOffset.y;
	localOffset.z = worldOffset.x * sinA + worldOffset.z * cosA;

	return localOffset;
}

void PlayerArmRush::UpdateRapidPunch()
{
	rushTimer_++;

	if (rushTimer_ % kRushInterval_ == 0) {
		rushAttackActive_ = true;
		rushAttackTimer_ = 0;
		rushCount_++;

		float sideOffset = (rushCount_ % 2 == 0) ? 0.5f : -0.5f;
		if (isRightArm_) { sideOffset += 0.3f; }
		else { sideOffset -= 0.3f; }

		Vector3 attackOffset = { sideOffset, 0.0f, kRushDistance_ };

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

	if (rushAttackActive_) {
		rushAttackTimer_++;
		float rushProgress = static_cast<float>(rushAttackTimer_) / static_cast<float>(kRushAttackDuration_);
		if (rushProgress >= 1.0f) { rushProgress = 1.0f; }

		float easedProgress = 1.0f - (1.0f - rushProgress) * (1.0f - rushProgress);
		if (rushProgress > 0.6f) {
			easedProgress = 1.0f - (rushProgress - 0.6f) * 2.5f;
		}

		currentTranslation_ = {
			originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
			originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
			originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
		};

		if (rushAttackTimer_ >= kRushAttackDuration_) {
			rushAttackActive_ = false;
			rushAttackTimer_ = 0;
			currentTranslation_ = originalPosition_;
		}
	}

	// 連続パンチフェーズ終了 → WindUp へ
	if (rushTimer_ >= kRushDuration_) {
		rushAttackActive_ = false;
		rushAttackTimer_ = 0;
		currentTranslation_ = originalPosition_;

		rushPhase_ = RushPhase::kWindUp;
		rushPhaseTimer_ = 0;

		// WindUp開始時点の体のY回転を記録
		bodyRotYAtWindUpStart_ = currentBodyRotY_;
	}
}

void PlayerArmRush::UpdateWindUp()
{
	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kWindUpDuration_);
	if (t > 1.0f) { t = 1.0f; }
	rushPhaseProgress_ = t;

	float eased = 1.0f - (1.0f - t) * (1.0f - t);

	Vector3 offset;
	if (isRightArm_) {
		offset = { kWindUpArmSideR_ * eased, 0.0f, kWindUpArmRetreat_ * eased };
	}
	else {
		offset = { 0.0f, 0.0f, kLArmWindUpZ_ * eased };
	}

	currentTranslation_ = {
		originalPosition_.x + offset.x,
		originalPosition_.y + offset.y,
		originalPosition_.z + offset.z
	};

	if (rushPhaseTimer_ >= kWindUpDuration_) {
		rushPhase_ = RushPhase::kFinisher;
		rushPhaseTimer_ = 0;
		finisherProgress_ = 0.0f;
		hasFinisherHit_ = false;

		if (isRightArm_) {
			targetPosition_ = {
				0.0f,
				originalPosition_.y,
				kFinisherArmAdvance_
			};
		}
		else {
			targetPosition_ = {
				0.0f,
				originalPosition_.y,
				kLArmFinisherZ_
			};
		}
	}
}

void PlayerArmRush::UpdateFinisher()
{
	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kFinisherDuration_);
	if (t > 1.0f) { t = 1.0f; }

	finisherProgress_ = t;
	rushPhaseProgress_ = t;

	if (isRightArm_) {
		float armEased;
		if (t <= 0.5f) {
			float t2 = t / 0.5f;
			armEased = t2 * t2;
		}
		else {
			float t2 = (t - 0.5f) / 0.5f;
			armEased = 1.0f - t2 * 0.25f;
		}

		Vector3 windUpEndLocal = {
			originalPosition_.x + kWindUpArmSideR_,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat_
		};

		Vector3 worldTargetOffset = {
			targetPosition_.x,
			0.0f,
			targetPosition_.z
		};
		Vector3 localTargetOffset = WorldOffsetToLocal(worldTargetOffset, currentBodyRotY_);
		Vector3 localTarget = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y + localTargetOffset.z,
			originalPosition_.z + localTargetOffset.z
		};
		localTarget.y = originalPosition_.y;
		localTarget.x = originalPosition_.x + localTargetOffset.x;
		localTarget.z = originalPosition_.z + localTargetOffset.z;

		currentTranslation_ = {
			windUpEndLocal.x + (localTarget.x - windUpEndLocal.x) * armEased,
			windUpEndLocal.y + (localTarget.y - windUpEndLocal.y) * armEased,
			windUpEndLocal.z + (localTarget.z - windUpEndLocal.z) * armEased
		};

		if (t >= 0.3f && t <= 0.7f && !hasFinisherHit_) {
			isFinisherHitFrame_ = true;
		}
	}
	else {
		float eased = t * (2.0f - t);	// ease-out

		Vector3 windUpEndLocal = {
			originalPosition_.x,
			originalPosition_.y,
			originalPosition_.z + kLArmWindUpZ_
		};

		Vector3 worldTargetOffset = {
			targetPosition_.x,
			0.0f,
			targetPosition_.z
		};
		Vector3 localTargetOffset = WorldOffsetToLocal(worldTargetOffset, currentBodyRotY_);
		Vector3 localTarget = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y,
			originalPosition_.z + localTargetOffset.z
		};

		currentTranslation_ = {
			windUpEndLocal.x + (localTarget.x - windUpEndLocal.x) * eased,
			windUpEndLocal.y + (localTarget.y - windUpEndLocal.y) * eased,
			windUpEndLocal.z + (localTarget.z - windUpEndLocal.z) * eased
		};
	}

	if (rushPhaseTimer_ >= kFinisherDuration_) {
		rushPhase_ = RushPhase::kRecover;
		rushPhaseTimer_ = 0;
	}
}

void PlayerArmRush::UpdateRecover()
{
	if (rushPhaseTimer_ >= 2) {
		isFinisherHitFrame_ = false;
	}

	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kRecoverDuration_);
	if (t > 1.0f) { t = 1.0f; }
	rushPhaseProgress_ = t;

	float eased = t * t;	// ease-in

	Vector3 worldTargetOffset = {
		targetPosition_.x,
		0.0f,
		targetPosition_.z
	};
	Vector3 localTargetOffset = WorldOffsetToLocal(worldTargetOffset, currentBodyRotY_);

	Vector3 finisherEndPos;
	if (isRightArm_) {
		Vector3 windUpEndLocal = {
			originalPosition_.x + kWindUpArmSideR_,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat_
		};
		Vector3 localTarget = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y,
			originalPosition_.z + localTargetOffset.z
		};
		finisherEndPos = {
			windUpEndLocal.x + (localTarget.x - windUpEndLocal.x) * 0.75f,
			windUpEndLocal.y + (localTarget.y - windUpEndLocal.y) * 0.75f,
			windUpEndLocal.z + (localTarget.z - windUpEndLocal.z) * 0.75f
		};
	}
	else {
		finisherEndPos = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y,
			originalPosition_.z + localTargetOffset.z
		};
	}

	currentTranslation_ = {
		finisherEndPos.x + (originalPosition_.x - finisherEndPos.x) * eased,
		finisherEndPos.y + (originalPosition_.y - finisherEndPos.y) * eased,
		finisherEndPos.z + (originalPosition_.z - finisherEndPos.z) * eased
	};

	if (rushPhaseTimer_ >= kRecoverDuration_) {
		// ラッシュ全体の終了
		isRush_ = false;
		rushTimer_ = 0;
		rushAttackTimer_ = 0;
		rushCount_ = 0;
		rushAttackActive_ = false;
		finisherProgress_ = 0.0f;
		isFinisherHitFrame_ = false;
		hasFinisherHit_ = false;
		rushPhase_ = RushPhase::kRapidPunch;
		rushPhaseTimer_ = 0;
		rushPhaseProgress_ = 0.0f;
		currentTranslation_ = originalPosition_;
		bodyRotYAtWindUpStart_ = 0.0f;
	}
}