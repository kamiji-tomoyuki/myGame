#include "PlayerArmRush.h"
#include <cmath>
#include <random.h>

using namespace Engine;
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
	variables_->AddItem(kGroupName_, "Rush Charge Duration", static_cast<int32_t>(kRushChargeDuration_));
	// フィニッシャーフェーズ
	variables_->AddItem(kGroupName_, "WindUp Duration", static_cast<int32_t>(kWindUpDuration_));
	variables_->AddItem(kGroupName_, "Finisher Duration", static_cast<int32_t>(kFinisherDuration_));
	variables_->AddItem(kGroupName_, "Recover Duration", static_cast<int32_t>(kRecoverDuration_));
	// 腕移動量
	variables_->AddItem(kGroupName_, "Rush Distance", kRushDistance_);
	variables_->AddItem(kGroupName_, "Charge Arm Pull", kChargeArmPull_);
	variables_->AddItem(kGroupName_, "Charge Arm Lift", kChargeArmLift_);
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
	kRushChargeDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Rush Charge Duration"));
	kWindUpDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "WindUp Duration"));
	kFinisherDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Finisher Duration"));
	kRecoverDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recover Duration"));
	kRushDistance_ = variables_->GetFloatValue(kGroupName_, "Rush Distance");
	kChargeArmPull_ = variables_->GetFloatValue(kGroupName_, "Charge Arm Pull");
	kChargeArmLift_ = variables_->GetFloatValue(kGroupName_, "Charge Arm Lift");
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
//  timerOffset — rushTimer_ の初期値。
//  左右の腕で異なるオフセットを渡すことで交互パンチを実現する。
//  例）右腕 = 0、左腕 = kRushInterval_ / 2
// =============================================================
void PlayerArmRush::StartRush(bool isRightArm, const Vector3& currentTranslation,
	uint32_t timerOffset)
{
	ApplyVariables();

	isRush_ = true;
	rapidPunchDone_ = false;
	isRightArm_ = isRightArm;
	rushPhase_ = RushPhase::kRapidPunch;
	isCharging_ = true; // 連打の前に「溜め」から開始
	chargeTimer_ = 0;
	rushTimer_ = timerOffset;   // ★ オフセットを初期値として設定
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

	// 溜め中は連打しない（体後傾＋腕を後ろへ引く）
	if (isCharging_) {
		UpdateCharge();
		return false;
	}

	switch (rushPhase_) {
	case RushPhase::kRapidPunch:
		UpdateRapidPunch();
		if (!isRush_) { return true; }	// 連打完了 → 腕のラッシュ終了（behaviorをkNormalへ）
		break;
	case RushPhase::kWindUp:     UpdateWindUp();     break;   // 未使用（フィニッシャーはクリップ化）
	case RushPhase::kFinisher:   UpdateFinisher();   break;   // 未使用
	case RushPhase::kRecover:
		UpdateRecover();
		if (!isRush_) { return true; }
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

void PlayerArmRush::UpdateCharge()
{
	chargeTimer_++;
	float t = (kRushChargeDuration_ > 0) ? static_cast<float>(chargeTimer_) / static_cast<float>(kRushChargeDuration_) : 1.0f;
	if (t > 1.0f) { t = 1.0f; }
	const float ease = t * t * (3.0f - 2.0f * t); // smoothstep

	// 両腕を少し後ろ(-Z)へ引き、下がらないよう少し上げる（ローカル姿勢）
	currentTranslation_ = {
		originalPosition_.x,
		originalPosition_.y + kChargeArmLift_ * ease,
		originalPosition_.z - kChargeArmPull_ * ease
	};

	if (chargeTimer_ >= kRushChargeDuration_) {
		isCharging_ = false;               // 溜め終了 → 連打へ
		currentTranslation_ = originalPosition_;
	}
}

void PlayerArmRush::UpdateRapidPunch()
{
	rushTimer_++;

	if (rushTimer_ % kRushInterval_ == 0) {
		rushAttackActive_ = true;
		rushAttackTimer_ = 0;
		rushCount_++;

		float sideOffset = (rushCount_ % 2 == 0) ? kRapidPunchSideBase_ : -kRapidPunchSideBase_;
		if (isRightArm_) { sideOffset += kRapidPunchArmSideOffset_; }
		else { sideOffset -= kRapidPunchArmSideOffset_; }

		// パンチごとにターゲット位置に大きなランダム性を加える
		Vector3 randomTargetOffset = {
			Random::Range(-kMaxRandomOffsetX_, kMaxRandomOffsetX_),
			Random::Range(-kMaxRandomOffsetY_, kMaxRandomOffsetY_),
			Random::Range(-kMaxRandomOffsetZ_, kMaxRandomOffsetZ_)
		};

		Vector3 attackOffset = { sideOffset + randomTargetOffset.x, randomTargetOffset.y, kRushDistance_ + randomTargetOffset.z };

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
		if (rushProgress > kRapidPunchEasingTurnPoint_) {
			easedProgress = 1.0f - (rushProgress - kRapidPunchEasingTurnPoint_) * kRapidPunchReturnSpeed_;
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

	// 連続パンチフェーズ終了 → 腕のラッシュは完了。
	// フィニッシャー（振りかぶり→大パンチ→戻り）は Player 側でモーションクリップとして再生する。
	// （旧: WindUp/Finisher/Recover の手続き処理は使用しない）
	if (rushTimer_ >= kRushDuration_) {
		rushAttackActive_ = false;
		rushAttackTimer_ = 0;
		currentTranslation_ = originalPosition_;

		isRush_ = false;          // 腕のラッシュ終了（behaviorはkNormalへ戻る）
		rapidPunchDone_ = true;   // Player がこれを見てフィニッシャークリップを開始する
		rushPhase_ = RushPhase::kRapidPunch;
		rushPhaseTimer_ = 0;
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
		if (t <= kFinisherHalfPoint_) {
			float t2 = t / kFinisherHalfPoint_;
			armEased = t2 * t2;
		}
		else {
			float t2 = (t - kFinisherHalfPoint_) / kFinisherHalfPoint_;
			armEased = 1.0f - t2 * kFinisherRetractFactor_;
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

		if (t >= kFinisherHitStartProgress_ && t <= kFinisherHitEndProgress_ && !hasFinisherHit_) {
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
	if (rushPhaseTimer_ >= kRecoverHitFrameClearDelay_) {
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
			windUpEndLocal.x + (localTarget.x - windUpEndLocal.x) * kRecoverStartRatio_,
			windUpEndLocal.y + (localTarget.y - windUpEndLocal.y) * kRecoverStartRatio_,
			windUpEndLocal.z + (localTarget.z - windUpEndLocal.z) * kRecoverStartRatio_
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