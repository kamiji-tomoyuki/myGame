#include "EnemyStateGameClear.h"
#include "Enemy.h"

void EnemyStateGameClear::Enter(Enemy* enemy)
{
	timer_ = 0.0f;
}

std::unique_ptr<IEnemyState> EnemyStateGameClear::Update(Enemy* enemy)
{
	timer_++;

	// 待機時間中は何もしない
	if (timer_ < kWaitTime_) {
		return nullptr;
	}

	// 待機後、回転しながら縮小
	Vector3 rot = enemy->GetObjRotation();
	rot.y += kRotationSpeed_;
	enemy->SetObjRotation(rot);
	enemy->SetRotation(rot);

	// 縮小処理
	Vector3 scale = enemy->GetWorldSize();
	if (scale.x >= 0.0f) {
		enemy->SetScale(Vector3(
			scale.x - kShrinkAmount_,
			scale.y - kShrinkAmount_,
			scale.z - kShrinkAmount_));
	}
	else {
		enemy->SetIsAlive(false);
	}

	return nullptr;
}