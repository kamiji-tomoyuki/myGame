#include "PlayerStateGameClear.h"
#include "Player.h"

void PlayerStateGameClear::Enter(Player* player)
{
	// クリア演出は gameClearEffect_ が自律動作するため特別な開始処理は不要
}

IPlayerState* PlayerStateGameClear::Update(Player* player)
{
	player->gameClearEffect_->Update();

	// ゲームクリアは終端状態 — 遷移しない
	return nullptr;
}
