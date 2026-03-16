#pragma once
#include "IPlayerState.h"

/// <summary>
/// ゲームクリア状態
/// </summary>
class PlayerStateGameClear : public IPlayerState
{
public:
	void Enter(Player* player) override;
	IPlayerState* Update(Player* player) override;
};
