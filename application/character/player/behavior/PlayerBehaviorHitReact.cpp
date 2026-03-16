#include "PlayerBehaviorHitReact.h"
#include "PlayerBehaviorRoot.h"
#include "Player.h"

void PlayerBehaviorHitReact::Enter(Player* player)
{
	// HitReaction は TakeDamage() 内で Start() 済み
}

IPlayerBehavior* PlayerBehaviorHitReact::Update(Player* player)
{
	// シェイクオフセットを計算（実座標は動かさない）
	player->hitReaction_->Update();

	// リアクションが終わったら通常行動へ戻る
	if (!player->hitReaction_->IsHitReacting()) {
		return new PlayerBehaviorRoot();
	}

	return nullptr; // 継続
}
