#include "EnemyStateGameClear.h"
#include "Enemy.h"

void EnemyStateGameClear::Enter(Enemy* enemy)
{
	timer_ = 0.0f;
}

IEnemyState* EnemyStateGameClear::Update(Enemy* enemy)
{
	timer_++;

	// 待機時間中は何もしない
	if (timer_ < kWaitTime_) {
		return nullptr;
	}

	// 待機後、回転しながら縮小
	Vector3 rot = enemy->GetObjRotation();
	rot.y += 0.5f;
	enemy->SetObjRotation(rot);
	enemy->SetRotation(rot);

	// 縮小処理
	Vector3 scale = enemy->GetWorldSize();
	if (scale.x >= 0.0f) {
		enemy->SetScale(Vector3(
			scale.x - 0.02f,
			scale.y - 0.02f,
			scale.z - 0.02f));
	}
	else {
		enemy->SetIsAlive(false);
	}

	return nullptr;
}
