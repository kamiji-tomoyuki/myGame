#include "PlayerStateGameClear.h"
#include "Player.h"

using namespace Engine;
void PlayerStateGameClear::Enter(Player* player)
{
	// クリア演出は gameClearEffect_ が自律動作するため特別な開始処理は不要
}

std::unique_ptr<IPlayerState> PlayerStateGameClear::Update(Player* player)
{
	player->gameClearEffect_->Update();

	// ゲームクリアは終端状態 — 遷移しない
	return nullptr;
}
