#include "PlayerArmRush.h"
#include <cmath>

// =============================================================
//  ラッシュ開始
// =============================================================
void PlayerArmRush::StartRush(bool isRightArm, const Vector3& currentTranslation)
{
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

	originalPosition_ = currentTranslation;
	currentTranslation_ = currentTranslation;
}

// =============================================================
//  更新 ― フェーズディスパッチャ
//  戻り値: ラッシュ全体が終了したら true
// =============================================================
bool PlayerArmRush::Update()
{
	if (!isRush_) { return false; }

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

// =============================================================
//  フェーズ1: 連続パンチ
// =============================================================
void PlayerArmRush::UpdateRapidPunch()
{
	rushTimer_++;

	if (rushTimer_ % kRushInterval == 0) {
		rushAttackActive_ = true;
		rushAttackTimer_ = 0;
		rushCount_++;

		float sideOffset = (rushCount_ % 2 == 0) ? 0.5f : -0.5f;
		if (isRightArm_) { sideOffset += 0.3f; }
		else { sideOffset -= 0.3f; }

		Vector3 attackOffset = { sideOffset, 0.0f, kRushDistance };

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
		float rushProgress = static_cast<float>(rushAttackTimer_) / static_cast<float>(kRushAttackDuration);
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

		if (rushAttackTimer_ >= kRushAttackDuration) {
			rushAttackActive_ = false;
			rushAttackTimer_ = 0;
			currentTranslation_ = originalPosition_;
		}
	}

	// 連続パンチフェーズ終了 → WindUp へ
	if (rushTimer_ >= kRushDuration) {
		rushAttackActive_ = false;
		rushAttackTimer_ = 0;
		currentTranslation_ = originalPosition_;

		rushPhase_ = RushPhase::kWindUp;
		rushPhaseTimer_ = 0;
	}
}

// =============================================================
//  フェーズ2: 振りかぶり
// =============================================================
void PlayerArmRush::UpdateWindUp()
{
	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kWindUpDuration);
	if (t > 1.0f) { t = 1.0f; }
	rushPhaseProgress_ = t;

	float eased = 1.0f - (1.0f - t) * (1.0f - t);

	Vector3 offset;
	if (isRightArm_) {
		offset = { kWindUpArmSideR * eased, 0.0f, kWindUpArmRetreat * eased };
	}
	else {
		offset = { 0.0f, 0.0f, kLArmWindUpZ * eased };
	}

	currentTranslation_ = {
		originalPosition_.x + offset.x,
		originalPosition_.y + offset.y,
		originalPosition_.z + offset.z
	};

	if (rushPhaseTimer_ >= kWindUpDuration) {
		rushPhase_ = RushPhase::kFinisher;
		rushPhaseTimer_ = 0;
		finisherProgress_ = 0.0f;
		hasFinisherHit_ = false;

		if (isRightArm_) {
			targetPosition_ = {
				originalPosition_.x,
				originalPosition_.y,
				originalPosition_.z + kFinisherArmAdvance
			};
		}
		else {
			targetPosition_ = {
				originalPosition_.x,
				originalPosition_.y,
				originalPosition_.z + kLArmFinisherZ
			};
		}
	}
}

// =============================================================
//  フェーズ3: フィニッシャー
// =============================================================
void PlayerArmRush::UpdateFinisher()
{
	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kFinisherDuration);
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

		Vector3 windUpEndPos = {
			originalPosition_.x + kWindUpArmSideR,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat
		};

		currentTranslation_ = {
			windUpEndPos.x + (targetPosition_.x - windUpEndPos.x) * armEased,
			windUpEndPos.y + (targetPosition_.y - windUpEndPos.y) * armEased,
			windUpEndPos.z + (targetPosition_.z - windUpEndPos.z) * armEased
		};

		// ヒット判定ウィンドウ（前進中: t=0.3〜0.7）
		// コライダーが敵に届くまでのラグを考慮して広めに設定
		if (t >= 0.3f && t <= 0.7f && !hasFinisherHit_) {
			isFinisherHitFrame_ = true;
		}
		else if (t > 0.7f) {
			// ウィンドウを抜けたら確実にオフ
			isFinisherHitFrame_ = false;
		}
	}
	else {
		float eased = t * (2.0f - t);	// ease-out

		Vector3 windUpEndPos = {
			originalPosition_.x,
			originalPosition_.y,
			originalPosition_.z + kLArmWindUpZ
		};

		currentTranslation_ = {
			windUpEndPos.x + (targetPosition_.x - windUpEndPos.x) * eased,
			windUpEndPos.y + (targetPosition_.y - windUpEndPos.y) * eased,
			windUpEndPos.z + (targetPosition_.z - windUpEndPos.z) * eased
		};
	}

	if (rushPhaseTimer_ >= kFinisherDuration) {
		rushPhase_ = RushPhase::kRecover;
		rushPhaseTimer_ = 0;
	}
}

// =============================================================
//  フェーズ4: 戻り
// =============================================================
void PlayerArmRush::UpdateRecover()
{
	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kRecoverDuration);
	if (t > 1.0f) { t = 1.0f; }
	rushPhaseProgress_ = t;

	float eased = t * t;	// ease-in

	Vector3 finisherEndPos;
	if (isRightArm_) {
		Vector3 windUpEndPos = {
			originalPosition_.x + kWindUpArmSideR,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat
		};
		finisherEndPos = {
			windUpEndPos.x + (targetPosition_.x - windUpEndPos.x) * 0.75f,
			windUpEndPos.y + (targetPosition_.y - windUpEndPos.y) * 0.75f,
			windUpEndPos.z + (targetPosition_.z - windUpEndPos.z) * 0.75f
		};
	}
	else {
		finisherEndPos = targetPosition_;
	}

	currentTranslation_ = {
		finisherEndPos.x + (originalPosition_.x - finisherEndPos.x) * eased,
		finisherEndPos.y + (originalPosition_.y - finisherEndPos.y) * eased,
		finisherEndPos.z + (originalPosition_.z - finisherEndPos.z) * eased
	};

	if (rushPhaseTimer_ >= kRecoverDuration) {
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
	}
}