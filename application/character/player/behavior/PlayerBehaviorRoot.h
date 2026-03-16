#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 通常行動（移動・攻撃・ロックオン）
/// </summary>
class PlayerBehaviorRoot : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	IPlayerBehavior* Update(Player* player) override;
};
