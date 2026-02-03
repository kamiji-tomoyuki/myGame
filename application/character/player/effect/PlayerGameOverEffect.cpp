#include "PlayerGameOverEffect.h"
#include "Player.h"

PlayerGameOverEffect::PlayerGameOverEffect(Player* player)
	: player_(player)
{
}

void PlayerGameOverEffect::Update()
{
	if (!isAlive_) {
		return;   // 既に演出完了
	}

	// モデルアニメーション & 腕の更新
	player_->UpdateModelAnimation();
	player_->UpdateArms();

	// Y軸を回転
	Vector3 rot = player_->GetWorldRotation();
	rot.y += kRotateSpeed_;
	player_->SetRotation(rot);

	// スケールを縮小
	Vector3 scale = player_->GetWorldSize();
	if (scale.x >= 0.0f) {
		scale.x -= kShrinkSpeed_;
		scale.y -= kShrinkSpeed_;
		scale.z -= kShrinkSpeed_;
		player_->SetScale(scale);
	}
	else {
		// サイズが負になった → 演出完了
		isAlive_ = false;
	}
}