#include "EnemyMove.h"
#include "Enemy.h"
#include "Player.h"
#include <cmath>

void EnemyMove::Update(Enemy* enemy, Player* player)
{
	if (player == nullptr) { return; }
	Approach(enemy, player);
	RecoverRotation(enemy);
}

void EnemyMove::Approach(Enemy* enemy, Player* player)
{
	Vector3 targetDirection = player->GetCenterPosition() - enemy->GetCenterPosition();
	float distanceToPlayer = targetDirection.Length();

	if (distanceToPlayer <= shortDistance_) {
		velocity_ = { 0.0f, 0.0f, 0.0f };
	}
	else {
		targetDirection = targetDirection.Normalize();
		Vector3 approachVelocity = targetDirection * approachSpeed_;

		velocity_ = velocity_ * 0.8f + approachVelocity * 0.2f;

		if (velocity_.Length() > maxSpeed_) {
			velocity_ = velocity_.Normalize() * maxSpeed_;
		}

		enemy->SetWorldPosition(enemy->GetCenterPosition() + velocity_);
	}

	// プレイヤーの方向を向く（距離に関係なく実行）
	if (distanceToPlayer > 0.0001f) {
		Vector3 normalizedDir = (player->GetCenterPosition() - enemy->GetCenterPosition()).Normalize();
		float targetRotY = std::atan2(normalizedDir.x, normalizedDir.z);

		Vector3 currentRot = enemy->GetCenterRotation();
		float currentRotY  = currentRot.y;

		float angleDiff = targetRotY - currentRotY;
		const float PI   = 3.14159265359f;
		while (angleDiff >  PI) { angleDiff -= 2.0f * PI; }
		while (angleDiff < -PI) { angleDiff += 2.0f * PI; }

		float newRotY = currentRotY + angleDiff * 0.1f;
		enemy->SetRotation(Vector3(currentRot.x, newRotY, currentRot.z));
		enemy->SetObjRotation(enemy->GetWorldRotation());
	}
}

void EnemyMove::RecoverRotation(Enemy* enemy)
{
	Vector3 currentRot  = enemy->GetObjRotation();
	Vector3 baseRot     = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();

	const float lerpFactor = 0.05f;
	Vector3 newRot = {
		currentRot.x + (originalRot.x - currentRot.x) * lerpFactor,
		baseRot.y,
		currentRot.z + (originalRot.z - currentRot.z) * lerpFactor
	};

	enemy->SetObjRotation(newRot);
}
