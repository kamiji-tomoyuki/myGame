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
	const float   kJumpCycle_ = 40.0f;
	const float   kJumpHeight_ = 2.0f;
	const float   kJumpPI_ = 3.14159265f;  // サイン計算用π
	const float   kRotationSpeed_ = 0.05f;        // Y軸回転速度
	const float   kTimerResetCycles_ = 1000.0f;      // タイマーリセット係数（サイクル数）
	const Vector3 kEnterScale_ = { 1.5f, 1.5f, 1.5f };
	const Vector3 kJumpEndPos_ = { 0.0f, 2.0f, 4.0f };
};