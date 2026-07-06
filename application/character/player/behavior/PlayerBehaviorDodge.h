#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 回避行動
/// </summary>
namespace Engine {}
using namespace Engine;
class PlayerBehaviorDodge : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerBehavior> Update(Player* player) override;
};
