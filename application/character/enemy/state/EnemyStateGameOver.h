#pragma once
#include "IEnemyState.h"
#include "Vector3.h"
#include <memory>
#include <numbers>

/// <summary>
/// ゲームオーバー状態
/// ジャンプし続ける演出を担う
/// </summary>
using namespace Engine;
class EnemyStateGameOver : public IEnemyState
{
public:
	void Enter(Enemy* enemy) override;
	std::unique_ptr<IEnemyState> Update(Enemy* enemy) override;

private:
	float timer_ = 0.0f;
	const float   kJumpCycle_ = 40.0f;
	const float   kJumpHeight_ = 2.0f;
	const float   kJumpPI_ = std::numbers::pi_v<float>;  // サイン計算用π
	const float   kRotationSpeed_ = 0.05f;        // Y軸回転速度
	const float   kTimerResetCycles_ = 1000.0f;      // タイマーリセット係数（サイクル数）
	const Vector3 kEnterScale_ = { 1.5f, 1.5f, 1.5f };
	const Vector3 kJumpEndPos_ = { 0.0f, 2.0f, 4.0f };
};