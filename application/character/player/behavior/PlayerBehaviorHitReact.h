#pragma once
#include "IPlayerBehavior.h"

/// <summary>
/// 被弾リアクション行動
/// </summary>
namespace Engine {}
using namespace Engine;
class PlayerBehaviorHitReact : public IPlayerBehavior
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerBehavior> Update(Player* player) override;
};
