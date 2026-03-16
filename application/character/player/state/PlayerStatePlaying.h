#pragma once
#include "IPlayerState.h"
#include <memory>

class IPlayerBehavior;

/// <summary>
/// プレイ中状態
/// 内部でさらに IPlayerBehavior（Root / Dodge / HitReact）を管理する
/// </summary>
class PlayerStatePlaying : public IPlayerState
{
public:
	void Enter(Player* player) override;
	IPlayerState* Update(Player* player) override;
	void Exit(Player* player) override;

private:
	/// <summary>Behavior を切り替える共通処理</summary>
	void ChangeBehavior(Player* player, IPlayerBehavior* next);

	std::unique_ptr<IPlayerBehavior> behavior_;
};
