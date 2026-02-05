#include "PlayerStartEffect.h"
#include "Player.h"
#include "myMath.h"   // Lerp

PlayerStartEffect::PlayerStartEffect(Player* player)
	: player_(player)
{
}

void PlayerStartEffect::Update()
{
	if (isEnd_) {
		return;   // 既に演出完了
	}

	player_->UpdateArms();

	if (easeT_ < 1.0f) {
		easeT_ += (1.0f / 60.0f) / kDuration_;
		if (easeT_ > 1.0f) {
			easeT_ = 1.0f;
		}
		player_->SetScale(Lerp(Vector3{ 0.0f, 0.0f, 0.0f },
			Vector3{ 1.0f, 1.0f, 1.0f },
			easeT_));
	}
	else {
		isEnd_ = true;
	}
}