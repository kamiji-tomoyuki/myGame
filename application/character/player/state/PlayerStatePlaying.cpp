#include "PlayerStatePlaying.h"
#include "PlayerStateGameOver.h"
#include "PlayerStateGameClear.h"
#include "PlayerBehaviorRoot.h"
#include "IPlayerBehavior.h"
#include "Player.h"

using namespace Engine;
void PlayerStatePlaying::Enter(Player* player)
{
	// 最初は通常行動から開始
	ChangeBehavior(player, std::make_unique<PlayerBehaviorRoot>());
}

std::unique_ptr<IPlayerState> PlayerStatePlaying::Update(Player* player)
{
	// ゲームオーバー遷移チェック
	if (player->GetGameState() == Player::GameState::kGameOver) {
		return std::make_unique<PlayerStateGameOver>();
	}

	// ゲームクリア遷移チェック
	if (player->GetGameState() == Player::GameState::kGameClear) {
		return std::make_unique<PlayerStateGameClear>();
	}

	player->hitReaction_->UpdateContactCooldown();

	// ロックオンが体の向きを読む前に、前フレームのコンボひねりを除去して純粋な facing に戻す
	player->RemoveComboBodyTwist();

	// Behavior の更新・遷移（この中で UpdateLockOn が facing を決める）
	if (behavior_) {
		std::unique_ptr<IPlayerBehavior> next = behavior_->Update(player);
		if (next) {
			ChangeBehavior(player, std::move(next));
		}
	}

	// facing 決定後・腕更新前にコンボの体ひねりを加算（腕は親=体を参照するため順序が重要）
	player->ApplyComboBodyTwist();

	// アニメーション・腕は常に更新
	player->obj3d_->UpdateAnimation(true);
	player->UpdateArms();

	// 必殺技モーション中はボディ回転の上書きをすべてスキップ
	//    （PlayerUltimate::FaceTarget が毎フレーム向きを制御するため）
	if (player->IsUltimateActive()) {
		return nullptr;
	}

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

	// フィニッシャー前進（フィニッシャークリップの体translateでプレイヤーを前進させる）
	player->ApplyFinisherBodyAdvance();

	return nullptr; // 継続
}

void PlayerStatePlaying::Exit(Player* player)
{
	if (behavior_) {
		behavior_->Exit(player);
		behavior_.reset();
	}
}

void PlayerStatePlaying::ChangeBehavior(Player* player, std::unique_ptr<IPlayerBehavior> next)
{
	if (behavior_) {
		behavior_->Exit(player);
	}
	behavior_ = std::move(next);
	if (behavior_) {
		behavior_->Enter(player);
	}
}