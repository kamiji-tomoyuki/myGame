#include "EnemyMove.h"
#include "Enemy.h"
#include "Player.h"
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

        velocity_.x = velocity_.x * 0.8f + approachVelocity.x * 0.2f;
        velocity_.y = 0.0f;
        velocity_.z = velocity_.z * 0.8f + approachVelocity.z * 0.2f;

        float speedXZ = std::sqrtf(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
        if (speedXZ > maxSpeed_) {
            float scale = maxSpeed_ / speedXZ;
            velocity_.x *= scale;
            velocity_.z *= scale;
        }

        Vector3 newPos = enemy->GetCenterPosition() + velocity_;
        newPos.y = groundY_;  // Y座標を地面に固定
        enemy->SetWorldPosition(newPos);
    }

    // プレイヤーの方向を向く（XZ平面内のみ）
    if (distanceToPlayer > 0.0001f) {
        Vector3 toPlayer = player->GetCenterPosition() - enemy->GetCenterPosition();
        float targetRotY = std::atan2(toPlayer.x, toPlayer.z);

        Vector3 currentRot = enemy->GetCenterRotation();
        float angleDiff = targetRotY - currentRot.y;
        const float PI = 3.14159265359f;
        while (angleDiff > PI) { angleDiff -= 2.0f * PI; }
        while (angleDiff < -PI) { angleDiff += 2.0f * PI; }

        float newRotY = currentRot.y + angleDiff * 0.1f;
        enemy->SetRotation(Vector3(currentRot.x, newRotY, currentRot.z));
        enemy->SetObjRotation(enemy->GetWorldRotation());
    }
}

void EnemyMove::RecoverRotation(Enemy* enemy)
{
    Vector3 currentRot = enemy->GetObjRotation();
    Vector3 baseRot = enemy->GetWorldRotation();
    Vector3 originalRot = enemy->GetOriginalRotation();

    const float lerpFactor = 0.05f;
    Vector3 newRot = {
        currentRot.x + (originalRot.x - currentRot.x) * lerpFactor,
        baseRot.y,
        currentRot.z + (originalRot.z - currentRot.z) * lerpFactor
    };

    enemy->SetObjRotation(newRot);
}