#include "PlayerBehaviorDodge.h"
#include "PlayerBehaviorRoot.h"
#include "Player.h"

using namespace Engine;
void PlayerBehaviorDodge::Enter(Player* player)
{
	// 回避開始時の処理は PlayerMove::IsDodgeRequested() で Start() 済み
}

std::unique_ptr<IPlayerBehavior> PlayerBehaviorDodge::Update(Player* player)
{
	player->dodge_->Update();

	// 回避が終わったら通常行動へ戻る
	if (!player->dodge_->IsDodging()) {
		return std::make_unique<PlayerBehaviorRoot>();
	}

	return nullptr; // 継続
}
