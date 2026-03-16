#pragma once
#include "IEnemyState.h"

/// <summary>
/// ゲームプレイ中の状態
/// Approach・攻撃管理・ラッシュ被弾処理を担う
/// </summary>
class EnemyStatePlaying : public IEnemyState
{
public:
	void Enter(Enemy* enemy) override;
	IEnemyState* Update(Enemy* enemy) override;
	void Exit(Enemy* enemy) override;
};
