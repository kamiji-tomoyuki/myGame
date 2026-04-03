#pragma once
#include "Vector3.h"
#include <cstdint>

class Enemy;
class Player;

/// <summary>
/// 敵のラッシュ被弾・スタン・ノックバック処理を担うコンポーネント
/// PlayerHitReactionに対応
/// </summary>
class EnemyHitReaction
{
public:
	EnemyHitReaction() = default;

	/// <summary>ラッシュ状態の変化を監視し、終了時のクリーンアップを行う</summary>
	void CheckPlayerRushStatus(Player* player);

	/// <summary>スタンタイマーを進める</summary>
	void UpdateStun();

	/// <summary>ラッシュヒット時に呼ばれる（Enemy::OnRushHit から委譲）</summary>
	void OnRushHit(bool isFinalHit, Enemy* enemy);

	/// <summary>ノックバック開始（EnemyStatePlaying から呼ぶ）</summary>
	void StartKnockback(Enemy* enemy, Player* player);

	/// <summary>ノックバック更新（EnemyStatePlaying から毎フレーム呼ぶ）</summary>
	void UpdateKnockback(Enemy* enemy);

	/// <summary>ノックバック終了</summary>
	void EndKnockback(Enemy* enemy);

	// --- ゲッター ---
	bool IsStunned()          const { return isRushStunned_; }
	bool IsBeingKnockedBack() const { return isBeingRushed_; }

private:
	// スタン
	bool  isRushStunned_ = false;
	int   rushStunTimer_ = 0;
	const int kRushStunDuration_ = 18;
	const int kFinalHitStunMultiplier_ = 3;     // ファイナルヒット時のスタン時間倍率

	// ノックバック
	bool     isBeingRushed_ = false;
	bool     rushFinalHitReceived_ = false;
	bool     wasRushActive_ = false;
	uint32_t rushKnockbackTimer_ = 0;

	Vector3 knockbackDirection_ = { 0.0f, 0.0f, 1.0f };
	float   knockbackSpeed_ = 0.02f;
	float   knockbackVerticalVelocity_ = 0.0f;
	float   knockbackGroundY_ = 0.0f;

	const float    initialKnockbackSpeed_ = 0.35f;
	const float    knockbackDecay_ = 0.88f;
	const float    maxTiltAngle_ = 0.5f;
	const float    knockbackInitialVerticalVelocity_ = 0.22f;
	const float    knockbackGravity_ = 0.012f;
	const uint32_t kMaxRushKnockbackDuration_ = 150;

	const float    kKnockbackDirectionMinLength_ = 0.001f;  // ノックバック方向の有効最小長さ
	const float    kAirborneThreshold_ = 0.01f;   // 空中判定の高さ閾値
	const float    kGroundTiltFrequency_ = 0.4f;    // 着地後揺れのサイン周波数
	const float    kGroundTiltDampingScale_ = 0.5f;    // 着地後揺れの減衰係数
};