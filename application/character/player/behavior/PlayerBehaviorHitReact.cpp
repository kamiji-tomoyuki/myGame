#include "PlayerBehaviorHitReact.h"
#include "PlayerBehaviorRoot.h"
#include "Player.h"

using namespace Engine;
void PlayerBehaviorHitReact::Enter(Player* player)
{
	// HitReaction は TakeDamage() 内で Start() 済み
}

std::unique_ptr<IPlayerBehavior> PlayerBehaviorHitReact::Update(Player* player)
{
	// シェイクオフセットを計算（実座標は動かさない）
	player->hitReaction_->Update();

	// リアクションが終わったら通常行動へ戻る
	if (!player->hitReaction_->IsHitReacting()) {
		return std::make_unique<PlayerBehaviorRoot>();
	}

	return nullptr; // 継続
}
