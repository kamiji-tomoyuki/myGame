#include "PlayerStatePlaying.h"
#include "PlayerStateGameOver.h"
#include "PlayerStateGameClear.h"
#include "PlayerBehaviorRoot.h"
#include "IPlayerBehavior.h"
#include "Player.h"

void PlayerStatePlaying::Enter(Player* player)
{
	// 最初は通常行動から開始
	ChangeBehavior(player, new PlayerBehaviorRoot());
}

IPlayerState* PlayerStatePlaying::Update(Player* player)
{
	// ゲームオーバー遷移チェック
	if (player->GetGameState() == Player::GameState::kGameOver) {
		return new PlayerStateGameOver();
	}

	// ゲームクリア遷移チェック
	if (player->GetGameState() == Player::GameState::kGameClear) {
		return new PlayerStateGameClear();
	}

	player->hitReaction_->UpdateContactCooldown();

	// Behavior の更新・遷移
	if (behavior_) {
		IPlayerBehavior* next = behavior_->Update(player);
		if (next) {
			ChangeBehavior(player, next);
		}
	}

	// アニメーション・腕・姿勢は Playing 中は常に更新
	player->obj3d_->UpdateAnimation(true);
	player->UpdateArms();

	// ラッシュ姿勢（ボディのひねり）
	{
		Vector3 currentRot = player->GetTransform().rotation_;
		Vector3 newRot = currentRot;
		player->rushPosture_->UpdateBodyPosture(player->arms_, currentRot, newRot);

		const float kTwistScale = 0.4f;
		newRot.y = currentRot.y + (newRot.y - currentRot.y) * kTwistScale;
		newRot.z = currentRot.z + (newRot.z - currentRot.z) * kTwistScale;
		player->SetRotation(newRot);
	}

	// フィニッシャー前進
	{
		Vector3 currentPos = player->GetWorldPosition();
		Vector3 newPos = currentPos;
		player->rushPosture_->UpdateFinisherAdvance(
			player->arms_,
			currentPos,
			player->GetTransform().rotation_.y,
			newPos);
		if (player->rushPosture_->IsFinisherAdvancing()) {
			player->SetWorldPosition(newPos);
		}
	}

	return nullptr; // 継続
}

void PlayerStatePlaying::Exit(Player* player)
{
	if (behavior_) {
		behavior_->Exit(player);
		behavior_.reset();
	}
}

void PlayerStatePlaying::ChangeBehavior(Player* player, IPlayerBehavior* next)
{
	if (behavior_) {
		behavior_->Exit(player);
	}
	behavior_.reset(next);
	if (behavior_) {
		behavior_->Enter(player);
	}
}