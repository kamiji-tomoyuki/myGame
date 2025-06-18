#include "StageManager.h"

StageManager* StageManager::GetInstance()
{
    static StageManager instance;
    return &instance;
}

void StageManager::Initialize()
{
    // デフォルト設定
    stageCenter_ = { 0.0f, 0.0f, 0.0f };  // ステージの中心座標
    stageRadius_ = 40.0f;                  // ステージの半径
}

bool StageManager::IsWithinStageBounds(const Vector3& position) const
{
    // Y軸は除外して平面上で判定（XZ平面）
    Vector3 stageCenter2D = { stageCenter_.x, 0.0f, stageCenter_.z };
    Vector3 position2D = { position.x, 0.0f, position.z };

    float distanceFromCenter = Vector3(position2D - stageCenter2D).Length();

    return distanceFromCenter <= stageRadius_;
}

Vector3 StageManager::ClampToStageBounds(const Vector3& position) const
{
    // Y軸は除外してXZ平面で判定・補正
    Vector3 stageCenter2D = { stageCenter_.x, 0.0f, stageCenter_.z };
    Vector3 position2D = { position.x, 0.0f, position.z };

    float distanceFromCenter = Vector3(position2D - stageCenter2D).Length();

    if (distanceFromCenter > stageRadius_) {
        // ステージ境界外の場合、境界上の点に補正
        if (distanceFromCenter > 0.0f) { // ゼロ除算回避
            Vector3 directionFromCenter = Vector3(position2D - stageCenter2D).Normalize();
            Vector3 clampedPosition2D = stageCenter2D + directionFromCenter * stageRadius_;

            // Y座標は元の値を保持
            return { clampedPosition2D.x, position.y, clampedPosition2D.z };
        }
        else {
            // 中心と同じ位置の場合は中心に配置
            return { stageCenter_.x, position.y, stageCenter_.z };
        }
    }

    return position;
}

float StageManager::GetDistanceFromCenter(const Vector3& position) const
{
    // Y軸を除外してXZ平面上での距離を計算
    Vector3 stageCenter2D = { stageCenter_.x, 0.0f, stageCenter_.z };
    Vector3 position2D = { position.x, 0.0f, position.z };

    return Vector3(position2D - stageCenter2D).Length();
}

float StageManager::GetDistanceFromBoundary(const Vector3& position) const
{
    float distanceFromCenter = GetDistanceFromCenter(position);
    return stageRadius_ - distanceFromCenter;
}