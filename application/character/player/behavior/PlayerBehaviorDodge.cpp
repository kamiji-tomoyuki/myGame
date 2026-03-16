#include "PlayerBehaviorDodge.h"
#include "PlayerBehaviorRoot.h"
#include "Player.h"

void PlayerBehaviorDodge::Enter(Player* player)
{
	// 回避開始時の処理は PlayerMove::IsDodgeRequested() で Start() 済み
}

IPlayerBehavior* PlayerBehaviorDodge::Update(Player* player)
{
	player->dodge_->Update();

	// 回避が終わったら通常行動へ戻る
	if (!player->dodge_->IsDodging()) {
		return new PlayerBehaviorRoot();
	}

	return nullptr; // 継続
}
