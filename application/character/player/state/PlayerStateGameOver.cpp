#include "PlayerStateGameOver.h"
#include "Player.h"

void PlayerStateGameOver::Enter(Player* player)
{
	// ゲームオーバー演出は gameOverEffect_ が自律動作するため特別な開始処理は不要
}

IPlayerState* PlayerStateGameOver::Update(Player* player)
{
	player->gameOverEffect_->Update();
	player->isAlive_ = player->gameOverEffect_->IsAlive();

	// ゲームオーバーは終端状態 — 遷移しない
	return nullptr;
}
