#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 回避行動
/// </summary>
class PlayerBehaviorDodge : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	IPlayerBehavior* Update(Player* player) override;
};
