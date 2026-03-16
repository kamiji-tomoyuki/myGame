#pragma once
#include "Vector3.h"

class Enemy;
class Player;

/// <summary>
/// 敵の移動・向き計算を担うコンポーネント
/// PlayerMoveに対応
/// </summary>
class EnemyMove
{
public:
	EnemyMove() = default;

	/// <summary>
	/// 毎フレームの移動更新
	/// Approach と RecoverRotation を内部で呼ぶ
	/// </summary>
	void Update(Enemy* enemy, Player* player);

	// --- ゲッター ---
	Vector3 GetVelocity()      const { return velocity_; }
	float   GetShortDistance() const { return shortDistance_; }

	// --- セッター ---
	void SetVelocity(const Vector3& v) { velocity_ = v; }

private:
	/// <summary>プレイヤーへ接近</summary>
	void Approach(Enemy* enemy, Player* player);

	/// <summary>通常回転に徐々に戻す</summary>
	void RecoverRotation(Enemy* enemy);

private:
	Vector3 velocity_     = { 0.0f, 0.0f, 0.0f };
	float shortDistance_  = 3.0f;
	float approachSpeed_  = 0.05f;
	float maxSpeed_       = 0.08f;
};
