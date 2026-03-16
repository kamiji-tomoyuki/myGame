#include "EnemyEffect.h"
#include "Enemy.h"
#include <Easing.h>

void EnemyEffect::UpdateStartEffect(Enemy* enemy)
{
	enemy->UpdateBaseObject();

	if (fallTimer_ < kFallDuration_) {
		Vector3 currentPos = EaseOutBounce<Vector3>(
			fallStartPos_,
			fallEndPos_,
			fallTimer_,
			kFallDuration_
		);
		enemy->SetWorldPosition(currentPos);
		fallTimer_++;

		// 落下中の回転演出
		float rotationSpeed = 0.1f;
		Vector3 rot = enemy->GetObjRotation();
		rot.y += rotationSpeed * (1.0f - (fallTimer_ / kFallDuration_));
		enemy->SetObjRotation(rot);
	}
	else {
		enemy->SetWorldPosition(fallEndPos_);
		isFallComplete_ = true;
	}
}
