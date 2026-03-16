#pragma once
#include "IEnemyState.h"
#include "Vector3.h"

/// <summary>
/// ゲームオーバー状態
/// ジャンプし続ける演出を担う
/// </summary>
class EnemyStateGameOver : public IEnemyState
{
public:
	void Enter(Enemy* enemy) override;
	IEnemyState* Update(Enemy* enemy) override;

private:
	float timer_ = 0.0f;
	static constexpr float kJumpCycle_  = 40.0f;
	static constexpr float kJumpHeight_ = 2.0f;
	const Vector3 kJumpEndPos_ = { 0.0f, 2.0f, 4.0f };
};
