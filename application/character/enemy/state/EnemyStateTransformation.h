#pragma once
#include "IEnemyState.h"
#include "Vector3.h"
#include <memory>

/// <summary>
/// 敵の強化演出状態
/// </summary>
using namespace Engine;
class EnemyStateTransformation : public IEnemyState
{
public:
	enum class Phase {
		kJumpUp,		// 飛び上がって画面外へ
		kLanding,		// フィールド中央に着地
		kFloating,		// 少し浮き上がる
		kTilting,       // 斜め上を向く
		kShaking,		// 2秒間横にシェイク
		kFacingForward, // 正面を向く
		kSlowLanding,	// ゆっくり着地
	};

public:
	void Enter(Enemy* enemy) override;
	std::unique_ptr<IEnemyState> Update(Enemy* enemy) override;
	void Exit(Enemy* enemy) override;

private:
	Phase phase_ = Phase::kJumpUp;
	uint32_t timer_ = 0;

	Vector3 startPos_;
	Vector3 targetPos_;

	const float kJumpHeight = 40.0f;
	const uint32_t kJumpTime = 45;
	const uint32_t kLandingTime = 30;
	const uint32_t kFloatTime = 30;
	const uint32_t kShakeTime = 120; // 2秒 (60fps想定)
	const uint32_t kSlowLandingTime = 60;

	const Vector3 kFieldCenter = { 0.0f, 1.2f, 15.0f };
	const float kFloatingHeight = 2.0f;
	const float kShakeAmount = 0.5f;
};
