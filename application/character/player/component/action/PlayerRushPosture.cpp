#include "PlayerRushPosture.h"
#include "FollowCamera.h"
#include <StageManager.h>
#include <cmath>

using namespace Engine;
void PlayerRushPosture::Init(FollowCamera* followCamera, StageManager* stageManager)
{
	followCamera_ = followCamera;
	stageManager_ = stageManager;
	rushBaseRotation_ = {};
	rushBodyPitchTarget_ = 0.0f;
	rushBodyPitchCurrent_ = 0.0f;
	rushBodyTwistTarget_ = 0.0f;
	rushBodyTwistCurrent_ = 0.0f;
}

// =============================================================
//  ラッシュ姿勢更新
// =============================================================
void PlayerRushPosture::UpdateBodyPosture(
	const std::array<std::unique_ptr<PlayerArm>, 2>& arms,
	const Vector3& currentRotation,
	Vector3& outRotation)
{
	const auto& rArm = arms[0];	// kRArm = 0
	if (!rArm) { outRotation = currentRotation; return; }

	bool inRush = rArm->GetIsRush();

	if (!inRush) {
		if (followCamera_) {
			followCamera_->SetFinisherMode(false);
		}

		// ラッシュ中でなければ姿勢をlerpで元に戻す
		rushBodyPitchCurrent_ *= 0.85f;
		rushBodyTwistCurrent_ *= 0.85f;

		if (std::abs(rushBodyPitchCurrent_) < 0.001f &&
			std::abs(rushBodyTwistCurrent_) < 0.001f) {
			rushBodyPitchCurrent_ = 0.0f;
			rushBodyTwistCurrent_ = 0.0f;
		}

		if (rushBodyPitchCurrent_ != 0.0f || rushBodyTwistCurrent_ != 0.0f) {
			Vector3 rot = rushBaseRotation_;
			rot.x += rushBodyPitchCurrent_;
			rot.y += rushBodyTwistCurrent_;
			outRotation = rot;
		}
		else {
			outRotation = currentRotation;
		}
		return;
	}

	// -------------------------------------------------------
	// フェーズ別ターゲット設定
	// -------------------------------------------------------
	PlayerArm::RushPhase phase = rArm->GetRushPhase();
	float                phaseProgress = rArm->GetRushPhaseProgress();

	bool needFinisherCam = (phase == PlayerArm::RushPhase::kWindUp ||
		phase == PlayerArm::RushPhase::kFinisher ||
		phase == PlayerArm::RushPhase::kRecover);

	if (followCamera_) {
		followCamera_->SetFinisherMode(needFinisherCam);
	}

	switch (phase) {
	case PlayerArm::RushPhase::kRapidPunch:
		rushBodyPitchTarget_ = kRushLeanPitch_;
		rushBodyTwistTarget_ = 0.0f;
		break;

	case PlayerArm::RushPhase::kWindUp:
		rushBodyPitchTarget_ = kRushLeanPitch_;
		rushBodyTwistTarget_ = kWindUpTwist_ * phaseProgress;
		break;

	case PlayerArm::RushPhase::kFinisher:
	{
		float fp = rArm->GetFinisherProgress();
		if (fp <= 0.5f) {
			float t = fp / 0.5f;
			rushBodyTwistTarget_ = kWindUpTwist_ + (kFinisherTwist_ - kWindUpTwist_) * (t * t);
			rushBodyPitchTarget_ = kRushLeanPitch_;
		}
		else {
			float t = (fp - 0.5f) / 0.5f;
			rushBodyTwistTarget_ = kFinisherTwist_ * (1.0f - t * 0.4f);
			rushBodyPitchTarget_ = kRushLeanPitch_ * (1.0f - t * 0.3f);
		}
		break;
	}

	case PlayerArm::RushPhase::kRecover:
		rushBodyPitchTarget_ = kRushLeanPitch_ * (1.0f - phaseProgress);
		rushBodyTwistTarget_ = kFinisherTwist_ * (1.0f - phaseProgress) * 0.6f;
		break;
	}

	float pitchSpeed = (phase == PlayerArm::RushPhase::kRapidPunch) ? 0.15f : 0.25f;
	float twistSpeed = 0.30f;

	rushBodyPitchCurrent_ += (rushBodyPitchTarget_ - rushBodyPitchCurrent_) * pitchSpeed;
	rushBodyTwistCurrent_ += (rushBodyTwistTarget_ - rushBodyTwistCurrent_) * twistSpeed;

	// 現在のY軸を基準に乗せる
	rushBaseRotation_ = { 0.0f, currentRotation.y, 0.0f };

	Vector3 newRot = rushBaseRotation_;
	newRot.x += rushBodyPitchCurrent_;
	newRot.y += rushBodyTwistCurrent_;
	outRotation = newRot;
}

// フィニッシャー前進はクリップ駆動へ移行（Player::ApplyFinisherBodyAdvance）。
// 旧 arm フェーズ依存の前進処理は撤去。