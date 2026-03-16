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
	bool  isRushStunned_  = false;
	int   rushStunTimer_  = 0;
	static constexpr int kRushStunDuration_ = 18;

	// ノックバック
	bool     isBeingRushed_         = false;
	bool     rushFinalHitReceived_  = false;
	bool     wasRushActive_         = false;
	uint32_t rushKnockbackTimer_    = 0;

	Vector3 knockbackDirection_          = { 0.0f, 0.0f, 1.0f };
	float   knockbackSpeed_              = 0.02f;
	float   knockbackVerticalVelocity_   = 0.0f;
	float   knockbackGroundY_            = 0.0f;

	static constexpr float initialKnockbackSpeed_          = 0.35f;
	static constexpr float knockbackDecay_                 = 0.88f;
	static constexpr float maxTiltAngle_                   = 0.5f;
	static constexpr float knockbackInitialVerticalVelocity_ = 0.22f;
	static constexpr float knockbackGravity_               = 0.012f;
	static constexpr uint32_t kMaxRushKnockbackDuration_  = 150;
};
