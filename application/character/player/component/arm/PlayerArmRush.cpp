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

		// WindUp開始時点の体のY回転を記録
		// （フィニッシャー中の補正の基準になる）
		bodyRotYAtWindUpStart_ = currentBodyRotY_;
	}
}

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
			// ワールド空間での目標オフセット（WindUp開始時の体向きを基準として前方）
			targetPosition_ = {
				0.0f,	// X: ひねりを逆補正するのでローカルXはほぼ0になる
				originalPosition_.y,
				kFinisherArmAdvance
			};
		}
		else {
			targetPosition_ = {
				0.0f,
				originalPosition_.y,
				kLArmFinisherZ
			};
		}
	}
}

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

		// WindUp終了時の腕位置（ローカル座標）を始点として補正
		// WindUp終了時点では体がまだ基準回転にあるのでローカル座標のまま使える
		Vector3 windUpEndLocal = {
			originalPosition_.x + kWindUpArmSideR,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat
		};

		// ターゲットはワールドオフセット（originalPosition_ からの前方 kFinisherArmAdvance）
		// → 毎フレーム現在の体回転を逆補正してローカル座標に変換
		Vector3 worldTargetOffset = {
			targetPosition_.x,
			0.0f,
			targetPosition_.z
		};
		Vector3 localTargetOffset = WorldOffsetToLocal(worldTargetOffset, currentBodyRotY_);
		Vector3 localTarget = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y + localTargetOffset.z,	// zは前後なのでyはそのまま
			originalPosition_.z + localTargetOffset.z
		};
		// y成分はひねりの影響を受けないのでそのまま
		localTarget.y = originalPosition_.y;
		localTarget.x = originalPosition_.x + localTargetOffset.x;
		localTarget.z = originalPosition_.z + localTargetOffset.z;

		currentTranslation_ = {
			windUpEndLocal.x + (localTarget.x - windUpEndLocal.x) * armEased,
			windUpEndLocal.y + (localTarget.y - windUpEndLocal.y) * armEased,
			windUpEndLocal.z + (localTarget.z - windUpEndLocal.z) * armEased
		};

		// ヒット判定ウィンドウ（前進中: t=0.3〜0.7）
		if (t >= 0.3f && t <= 0.7f && !hasFinisherHit_) {
			isFinisherHitFrame_ = true;
		}
	}
	else {
		// 左腕もひねり補正を適用（左腕は引き気味なので補正量は小さい）
		float eased = t * (2.0f - t);	// ease-out

		Vector3 windUpEndLocal = {
			originalPosition_.x,
			originalPosition_.y,
			originalPosition_.z + kLArmWindUpZ
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

	if (rushPhaseTimer_ >= kFinisherDuration) {
		rushPhase_ = RushPhase::kRecover;
		rushPhaseTimer_ = 0;
	}
}

void PlayerArmRush::UpdateRecover()
{
	if (rushPhaseTimer_ >= 2) {
		isFinisherHitFrame_ = false;
	}

	float t = static_cast<float>(rushPhaseTimer_) / static_cast<float>(kRecoverDuration);
	if (t > 1.0f) { t = 1.0f; }
	rushPhaseProgress_ = t;

	float eased = t * t;	// ease-in

	// フィニッシャー終了時の腕のワールドオフセットをローカルに補正して戻す
	Vector3 worldTargetOffset = {
		targetPosition_.x,
		0.0f,
		targetPosition_.z
	};
	Vector3 localTargetOffset = WorldOffsetToLocal(worldTargetOffset, currentBodyRotY_);

	Vector3 finisherEndPos;
	if (isRightArm_) {
		Vector3 windUpEndLocal = {
			originalPosition_.x + kWindUpArmSideR,
			originalPosition_.y,
			originalPosition_.z + kWindUpArmRetreat
		};
		Vector3 localTarget = {
			originalPosition_.x + localTargetOffset.x,
			originalPosition_.y,
			originalPosition_.z + localTargetOffset.z
		};
		// フィニッシャーの 75% 地点を戻りの起点にする（元のロジックと同じ割合）
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
		bodyRotYAtWindUpStart_ = 0.0f;
	}
}