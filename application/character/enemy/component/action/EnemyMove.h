#pragma once
#include "Vector3.h"

class Enemy;
class Player;

/// <summary>
/// 敵の移動・向き計算を担うコンポーネント
/// </summary>
class EnemyMove
{
public:
    EnemyMove() = default;

    void Update(Enemy* enemy, Player* player);

    Vector3 GetVelocity()      const { return velocity_; }
    float   GetShortDistance() const { return shortDistance_; }
    void    SetVelocity(const Vector3& v) { velocity_ = v; }

private:
    void Approach(Enemy* enemy, Player* player);
    void RecoverRotation(Enemy* enemy);

private:
    Vector3 velocity_ = {};
    float   shortDistance_ = 3.0f;
    float   approachSpeed_ = 0.05f;
    float   maxSpeed_ = 0.08f;

    // 地面Y座標の記録（Init後の最初のフレームで確定）
    float groundY_ = 0.0f;
    bool  groundYInitialized_ = false;

    const float kVelocitySmoothingKeep_ = 0.8f;    // 速度スムージング：現在値の保持割合
    const float kVelocitySmoothingNew_ = 0.2f;    // 速度スムージング：新規値の反映割合
    const float kRotationMinDistance_ = 0.0001f; // 回転計算を行う最小プレイヤー距離
    const float kRotationSmoothingFactor_ = 0.1f;    // 回転スムージング係数
    const float kRecoverRotationLerp_ = 0.05f;   // 回転復帰のlerp係数
    const float kPI_ = 3.14159265359f;
};