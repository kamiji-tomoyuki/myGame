#include "PlayerGameClearEffect.h"
#include "Player.h"
#include <cmath>   // fmod, sinf

PlayerGameClearEffect::PlayerGameClearEffect(Player* player)
	: player_(player)
{
}

void PlayerGameClearEffect::Update()
{
	// モデルアニメーション & 腕の更新
	player_->UpdateModelAnimation();
	player_->UpdateArms();

	// 初回実行時に開始位置を保存
	if (!initialized_) {
		clearStartPos_ = player_->GetWorldPosition();
		initialized_ = true;
	}

	// ジャンプフェーズ（0.0〜1.0の周期）
	float jumpPhase = fmod(timer_, kJumpCycle_) / kJumpCycle_;

	// sin波による滑らかなジャンプオフセット
	float jumpOffset = sinf(jumpPhase * 3.14159265f) * kJumpHeight_;

	// 位置を設定
	Vector3 pos = clearStartPos_;
	pos.y += jumpOffset;
	player_->SetWorldPosition(pos);

	// Y軸ゆっくり回転
	Vector3 rot = player_->GetWorldRotation();
	rot.y += kRotateSpeed_;
	player_->SetRotation(rot);

	// タイマー進行
	timer_++;

	// オーバーフロー防止
	if (timer_ >= kTimerResetThreshold_) {
		timer_ = 0.0f;
	}
}