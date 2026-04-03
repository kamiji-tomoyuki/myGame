#include "EnemyStateGameOver.h"
#include "Enemy.h"
#include <cmath>

void EnemyStateGameOver::Enter(Enemy* enemy)
{
	timer_ = 0.0f;
	enemy->SetScale(kEnterScale_);
}

std::unique_ptr<IEnemyState> EnemyStateGameOver::Update(Enemy* enemy)
{
	// 現在のジャンプフェーズ (0.0 ~ 1.0)
	float jumpPhase = fmod(timer_, kJumpCycle_) / kJumpCycle_;
	float jumpOffset = sin(jumpPhase * kJumpPI_) * kJumpHeight_;

	Vector3 currentPos = kJumpEndPos_;
	currentPos.y += jumpOffset;
	enemy->SetWorldPosition(currentPos);

	// Y軸回転アニメーション
	Vector3 rot = enemy->GetObjRotation();
	rot.y += kRotationSpeed_;
	enemy->SetObjRotation(rot);

	timer_++;
	if (timer_ >= kJumpCycle_ * kTimerResetCycles_) {
		timer_ = 0.0f;
	}

	return nullptr; // GameOver から他状態へは遷移しない
}