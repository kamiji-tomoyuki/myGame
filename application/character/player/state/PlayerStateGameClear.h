#pragma once
#include "IPlayerState.h"

/// <summary>
/// ゲームクリア状態
/// </summary>
using namespace Engine;
class PlayerStateGameClear : public IPlayerState
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerState> Update(Player* player) override;
};
