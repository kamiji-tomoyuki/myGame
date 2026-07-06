#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 通常行動（移動・攻撃・ロックオン）
/// </summary>
namespace Engine {}
using namespace Engine;
class PlayerBehaviorRoot : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerBehavior> Update(Player* player) override;
};
