#pragma once
#include "IPlayerState.h"

/// <summary>
/// ゲームオーバー状態
/// </summary>
class PlayerStateGameOver : public IPlayerState
{
public:
	void Enter(Player* player) override;
	IPlayerState* Update(Player* player) override;
};
