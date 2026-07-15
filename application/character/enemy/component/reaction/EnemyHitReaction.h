#pragma once
#include "Vector3.h"
#include <cstdint>

using namespace Engine;
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
	void CheckPlayerRushStatus(Enemy* enemy);

	/// <summary>スタンタイマーを進める</summary>
	void UpdateStun();

	/// <summary>ラッシュヒット時に呼ばれる（Enemy::OnRushHit から委譲）</summary>
	void OnRushHit(bool isFinalHit, Enemy* enemy);

	/// <summary>ノックバック開始（EnemyStatePlaying から呼ぶ）</summary>
	void StartKnockback(Enemy* enemy, Player* player, bool shouldBounce = false);

	/// <summary>ノックバック更新（EnemyStatePlaying から毎フレーム呼ぶ）</summary>
	void UpdateKnockback(Enemy* enemy);

	/// <summary>ノックバック終了</summary>
	void EndKnockback(Enemy* enemy);

	/// <summary>被弾時：赤フラッシュ開始＋連続ヒットカウント加算</summary>
	void OnHit();

	/// <summary>赤フラッシュの更新（毎フレーム）</summary>
	void UpdateFlash();

	/// <summary>連続ヒット対策：一定連続で被弾したら攻撃を中断してプレイヤーを押し返す。
	///          EnemyStatePlaying から毎フレーム呼ぶ。</summary>
	void UpdateComboRepel(Enemy* enemy);

	/// <summary>被弾リアクション（フラッシュ・スタン・ノックバック・連続ヒット）を全てリセットする</summary>
	void Reset(Enemy* enemy);

	// --- ゲッター ---
	bool  IsStunned()          const { return isRushStunned_; }
	bool  IsBeingKnockedBack() const { return isBeingRushed_; }
	bool  IsFlashing()         const { return isFlashing_; }
	/// <summary>フラッシュ強度[0,1]（1=最も赤い）</summary>
	float GetFlashIntensity()  const { return (kFlashDuration_ > 0) ? static_cast<float>(flashTimer_) / static_cast<float>(kFlashDuration_) : 0.0f; }

	private:
	/// <summary>連続ヒット閾値に達したときの押し返し発動</summary>
	void DoRepel(Enemy* enemy, Player* player);

	// スタン
	bool  isRushStunned_ = false;
	int   rushStunTimer_ = 0;
	const int kRushStunDuration_ = 18;
	const int kFinalHitStunMultiplier_ = 3;     // ファイナルヒット時のスタン時間倍率

	// 被弾リアクション（赤フラッシュ）
	bool  isFlashing_ = false;
	int   flashTimer_ = 0;
	const int kFlashDuration_ = 8; // 赤フラッシュのフレーム数

	// 連続ヒット対策のノックバック（プレイヤーを押し返して距離をとらせる）
	int     consecutiveHits_ = 0;
	int     hitWindow_ = 0;       // 0でリセット（この猶予内の連続ヒットを数える）
	int     repelCooldown_ = 0;   // 再発動までのクールダウン
	int     repelPushTimer_ = 0;  // プレイヤー押し出しの残りフレーム
	Vector3 repelDir_ = { 0.0f, 0.0f, -1.0f };
	const int   kComboRepelThreshold_ = 6;   // 何連続ヒットで発動するか（連続攻撃を受け続けたら押し返す）
	const int   kHitWindowFrames_ = 120;     // 連続ヒットの猶予（無ヒットでリセット。ラッシュ+αを跨げる長さ）
	const int   kRepelCooldownFrames_ = 120; // 発動後のクールダウン
	const int   kRepelPushFrames_ = 14;      // 押し出し継続フレーム
	const float kRepelPushSpeed_ = 0.55f;    // 1フレームあたりの押し出し量

	// ノックバック
	bool     isBeingRushed_ = false;
	bool     rushFinalHitReceived_ = false;
	bool     wasRushActive_ = false;
	uint32_t rushKnockbackTimer_ = 0;
	bool     isBouncing_ = false;

	Vector3 knockbackDirection_ = { 0.0f, 0.0f, 1.0f };
	float   knockbackSpeed_ = 0.02f;
	float   knockbackVerticalVelocity_ = 0.0f;
	float   knockbackGroundY_ = 0.0f;

	const float    initialKnockbackSpeed_ = 0.8f;
	const float    knockbackDecay_ = 0.92f;
	const float    maxTiltAngle_ = 0.5f;
	const float    knockbackInitialVerticalVelocity_ = 0.45f;
	const float    knockbackGravity_ = 0.02f;
	const uint32_t kMaxRushKnockbackDuration_ = 150;

	const float    kKnockbackDirectionMinLength_ = 0.001f;  // ノックバック方向の有効最小長さ
	const float    kAirborneThreshold_ = 0.01f;   // 空中判定の高さ閾値
	const float    kGroundTiltFrequency_ = 0.4f;    // 着地後揺れのサイン周波数
	const float    kGroundTiltDampingScale_ = 0.5f;    // 着地後揺れの減衰係数

	const float    kBounceFactor_ = 0.5f;          // バウンド時の速度減衰率
	const float    kMinBounceVelocity_ = 0.05f;    // バウンドを終了する最小速度
	};