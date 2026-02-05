#include "PlayerDodge.h"
#include "Player.h"
#include "StageManager.h"
#include "myMath.h"   // MakeRotateYMatrix, TransformNormal
#include <cmath>      // std::abs

PlayerDodge::PlayerDodge(Player* player, StageManager* stageManager)
	: player_(player)
	, stageManager_(stageManager)
{
}

// =============================================================
// Start
// =============================================================
void PlayerDodge::Start(const Vector3& localDirection)
{
	isDodging_ = true;
	timer_ = 0;
	startRotation_ = player_->GetWorldRotation();

	// ローカル入力を正規化
	Vector3 dir = localDirection;
	if (dir.Length() > 0.0f) {
		dir = dir.Normalize();
	}

	// プレイヤーの向きに合わせてワールド空間に変換
	float   rotY = startRotation_.y;
	Matrix4x4 rotMat = MakeRotateYMatrix(rotY);
	dodgeDirection_ = TransformNormal(dir, rotMat);

	// 傾き方向を計算
	CalcTiltRotation(dodgeDirection_);

	// 移動速度をリセット
	player_->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
}

// =============================================================
// Update
// =============================================================
void PlayerDodge::Update()
{
	if (!isDodging_) {
		return;
	}

	timer_++;

	// -------------------------------------------------------
	// 傾き補間（入り・中央・出し）
	// -------------------------------------------------------
	float tiltProgress = 0.0f;

	if (timer_ < kTiltInDuration_) {
		// 入り：ease-out (1 - (1-t)^2)
		float t = static_cast<float>(timer_) / kTiltInDuration_;
		tiltProgress = 1.0f - (1.0f - t) * (1.0f - t);
	}
	else if (timer_ >= kDuration_ - kTiltOutDuration_) {
		// 出し：ease-in (t^2)
		int   outTimer = timer_ - (kDuration_ - kTiltOutDuration_);
		float t = static_cast<float>(outTimer) / kTiltOutDuration_;
		tiltProgress = 1.0f - (t * t);
	}
	else {
		// 中央：傾き全量
		tiltProgress = 1.0f;
	}

	// 傾き回転を適用（Y軸は開始時の値を維持）
	Vector3 currentTilt = tiltRotation_ * tiltProgress;
	Vector3 rot;
	rot.x = currentTilt.x;
	rot.y = startRotation_.y;
	rot.z = currentTilt.z;
	player_->SetRotation(rot);

	// -------------------------------------------------------
	// 位置移動
	// -------------------------------------------------------
	Vector3 pos = player_->GetWorldPosition();
	Vector3 newPos = pos + dodgeDirection_ * kSpeed_;

	// ステージ境界クランプ
	if (stageManager_ != nullptr) {
		if (!stageManager_->IsWithinStageBounds(newPos)) {
			newPos = stageManager_->ClampToStageBounds(newPos);
		}
	}

	player_->SetWorldPosition(newPos);

	// -------------------------------------------------------
	// 終了チェック
	// -------------------------------------------------------
	if (timer_ >= kDuration_) {
		isDodging_ = false;
		timer_ = 0;
		// 傾きを元に戻す
		player_->SetRotation(Vector3(0.0f, startRotation_.y, 0.0f));
	}
}

// =============================================================
// CalcTiltRotation（内部ヘルパー）
// =============================================================
void PlayerDodge::CalcTiltRotation(const Vector3& worldDirection)
{
	float worldX = worldDirection.x;
	float worldZ = worldDirection.z;
	float absX = std::abs(worldX);
	float absZ = std::abs(worldZ);

	// 主軸方向の傾き
	if (absZ > absX) {
		tiltRotation_ = Vector3(worldZ > 0.0f ? kTiltAngle_ : -kTiltAngle_,
			0.0f, 0.0f);
	}
	else {
		tiltRotation_ = Vector3(0.0f, 0.0f,
			worldX > 0.0f ? -kTiltAngle_ : kTiltAngle_);
	}

	// 斜め方向の場合は両軸に傾き
	if (absX > 0.3f && absZ > 0.3f) {
		float xTilt = worldZ > 0.0f ? kTiltAngle_ * 0.7f : -kTiltAngle_ * 0.7f;
		float zTilt = worldX > 0.0f ? -kTiltAngle_ * 0.7f : kTiltAngle_ * 0.7f;
		tiltRotation_ = Vector3(xTilt, 0.0f, zTilt);
	}
}