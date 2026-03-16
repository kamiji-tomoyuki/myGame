#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 被弾リアクション行動
/// </summary>
class PlayerBehaviorHitReact : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	IPlayerBehavior* Update(Player* player) override;
};
