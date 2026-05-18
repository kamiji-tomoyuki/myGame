#include "EnemyMove.h"
#include "Enemy.h"
#include "Player.h"
#include "StageManager.h"
#include <ParticleEmitter.h>
#include <cmath>

void EnemyMove::Update(Enemy* enemy, Player* player)
{
    if (player == nullptr) { return; }

    // 初回フレームで地面Y座標を記録する
    if (!groundYInitialized_) {
        groundY_ = enemy->GetCenterPosition().y;
        groundYInitialized_ = true;
    }

    Approach(enemy, player);
    RecoverRotation(enemy);
    UpdateTrailEffect(enemy->GetCenterPosition());
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

        // XZ 成分のみで接近（Y は無視）
        Vector3 approachVelocity = {
            targetDirection.x * approachSpeed_,
            0.0f,
            targetDirection.z * approachSpeed_
        };

        velocity_.x = velocity_.x * kVelocitySmoothingKeep_ + approachVelocity.x * kVelocitySmoothingNew_;
        velocity_.y = 0.0f;
        velocity_.z = velocity_.z * kVelocitySmoothingKeep_ + approachVelocity.z * kVelocitySmoothingNew_;

        float speedXZ = std::sqrtf(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
        if (speedXZ > maxSpeed_) {
            float scale = maxSpeed_ / speedXZ;
            velocity_.x *= scale;
            velocity_.z *= scale;
        }

        Vector3 newPos = enemy->GetCenterPosition() + velocity_;
        newPos.y = groundY_;  // Y座標を地面に固定
        
        // ステージ境界内に制限
        newPos = StageManager::GetInstance()->ClampToStageBounds(newPos);

        enemy->SetWorldPosition(newPos);
    }

    // プレイヤーの方向を向く（XZ平面内のみ）
    if (distanceToPlayer > kRotationMinDistance_) {
        Vector3 toPlayer = player->GetCenterPosition() - enemy->GetCenterPosition();
        float targetRotY = std::atan2(toPlayer.x, toPlayer.z);

        Vector3 currentRot = enemy->GetCenterRotation();
        float angleDiff = targetRotY - currentRot.y;
        while (angleDiff > kPI_) { angleDiff -= 2.0f * kPI_; }
        while (angleDiff < -kPI_) { angleDiff += 2.0f * kPI_; }

        float newRotY = currentRot.y + angleDiff * kRotationSmoothingFactor_;
        enemy->SetRotation(Vector3(currentRot.x, newRotY, currentRot.z));
        enemy->SetObjRotation(enemy->GetWorldRotation());
    }
}

// =============================================================
//  軌跡エフェクト更新
// =============================================================
void EnemyMove::UpdateTrailEffect(const Vector3& currentWorldPos)
{
    if (!trailEffect_) { return; }

    Vector3 footPosition = currentWorldPos;
    footPosition.y += kFootOffsetY_;

    if (velocity_.Length() > 0.01f) {
        trailEffect_->SetPosition(footPosition);
        trailEffect_->SetActive(true);
        isTrailActive_ = true;
    }
    else {
        trailEffect_->SetActive(false);
        isTrailActive_ = false;
    }
}

void EnemyMove::RecoverRotation(Enemy* enemy) {
    Vector3 currentRot = enemy->GetObjRotation();
    Vector3 baseRot = enemy->GetWorldRotation();
    Vector3 originalRot = enemy->GetOriginalRotation();

    const float lerpFactor = kRecoverRotationLerp_;
    Vector3 newRot = {
        currentRot.x + (originalRot.x - currentRot.x) * lerpFactor,
        baseRot.y,
        currentRot.z + (originalRot.z - currentRot.z) * lerpFactor
    };

    enemy->SetObjRotation(newRot);
}