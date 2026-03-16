#include "EnemyStateGameOver.h"
#include "Enemy.h"
#include <cmath>

void EnemyStateGameOver::Enter(Enemy* enemy)
{
	timer_ = 0.0f;
	enemy->SetScale(Vector3(1.5f, 1.5f, 1.5f));
}

IEnemyState* EnemyStateGameOver::Update(Enemy* enemy)
{
	// 現在のジャンプフェーズ (0.0 ~ 1.0)
	float jumpPhase = fmod(timer_, kJumpCycle_) / kJumpCycle_;
	float jumpOffset = sin(jumpPhase * 3.14159265f) * kJumpHeight_;

	Vector3 currentPos = kJumpEndPos_;
	currentPos.y += jumpOffset;
	enemy->SetWorldPosition(currentPos);

	// Y軸回転アニメーション
	Vector3 rot = enemy->GetObjRotation();
	rot.y += 0.05f;
	enemy->SetObjRotation(rot);

	timer_++;
	if (timer_ >= kJumpCycle_ * 1000.0f) {
		timer_ = 0.0f;
	}

	return nullptr; // GameOver から他状態へは遷移しない
}
