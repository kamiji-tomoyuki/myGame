#pragma once
#include "IPlayerState.h"

/// <summary>
/// ゲームオーバー状態
/// </summary>
using namespace Engine;
class PlayerStateGameOver : public IPlayerState
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerState> Update(Player* player) override;
};
