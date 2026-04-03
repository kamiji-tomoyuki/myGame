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
	const float kWaitTime_ = 1.2f * 60.0f;
	const float kRotationSpeed_ = 0.5f;   // 縮小中のY軸回転速度
	const float kShrinkAmount_ = 0.02f;  // 1フレームあたりの縮小量
};