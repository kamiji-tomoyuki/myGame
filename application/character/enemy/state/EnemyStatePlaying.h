#pragma once
#include "IEnemyState.h"
#include <memory>

/// <summary>
/// ゲームプレイ中の状態
/// Approach・攻撃管理・ラッシュ被弾処理を担う
/// </summary>
class EnemyStatePlaying : public IEnemyState
{
public:
	void Enter(Enemy* enemy) override;
	std::unique_ptr<IEnemyState> Update(Enemy* enemy) override;
	void Exit(Enemy* enemy) override;
};