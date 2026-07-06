#include "PlayerBehaviorRoot.h"
#include "PlayerBehaviorDodge.h"
#include "PlayerBehaviorHitReact.h"
#include "Player.h"

using namespace Engine;
void PlayerBehaviorRoot::Enter(Player* player)
{
	// 通常状態に入ったとき特別な処理は不要
}

std::unique_ptr<IPlayerBehavior> PlayerBehaviorRoot::Update(Player* player)
{
	// 被弾リアクションへの遷移チェック
	if (player->hitReaction_->IsHitReacting()) {
		return std::make_unique<PlayerBehaviorHitReact>();
	}

	// 回避への遷移チェック
	if (player->dodge_->IsDodging()) {
		return std::make_unique<PlayerBehaviorDodge>();
	}

	// 通常処理
	player->hitEffect_->SetPosition(player->GetWorldPosition());
	player->MoveInternal();
	player->attack_->Update();
	player->UpdateLockOn();

	return nullptr; // 継続
}
