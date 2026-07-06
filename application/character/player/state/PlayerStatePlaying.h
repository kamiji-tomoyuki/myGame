#pragma once
#include "IPlayerState.h"
#include "IPlayerBehavior.h"
#include <memory>

/// <summary>
/// プレイ中状態
/// 内部でさらに IPlayerBehavior（Root / Dodge / HitReact）を管理する
/// </summary>
using namespace Engine;
class PlayerStatePlaying : public IPlayerState
{
public:
	void Enter(Player* player) override;
	std::unique_ptr<IPlayerState> Update(Player* player) override;
	void Exit(Player* player) override;

private:
	/// <summary>Behavior を切り替える共通処理</summary>
	void ChangeBehavior(Player* player, std::unique_ptr<IPlayerBehavior> next);

	std::unique_ptr<IPlayerBehavior> behavior_;
};