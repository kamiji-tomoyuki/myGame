#pragma once
#include "IEnemyState.h"
#include "Vector3.h"

/// <summary>
/// ゲームクリア状態
/// 待機後、回転しながら縮小して消滅する演出を担う
/// </summary>
class EnemyStateGameClear : public IEnemyState
{
public:
	void Enter(Enemy* enemy) override;
	IEnemyState* Update(Enemy* enemy) override;

private:
	float timer_ = 0.0f;
	static constexpr float kWaitTime_ = 1.2f * 60.0f;
};
