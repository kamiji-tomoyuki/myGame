#pragma once
#include "Vector3.h"
#include <memory>

class Enemy;
class Player;
class ParticleEmitter;

class EnemyAttackMelee
{
public:
	enum class Phase {
		kNone,
		kPreparation,	// 予備動作
		kCharging,		// 突進中
		kRecovery,		// 硬直(回復)
	};

public:
	EnemyAttackMelee();
	~EnemyAttackMelee();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 攻撃開始
	/// </summary>
	void Start(Enemy* enemy, Player* player);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Enemy* enemy, Player* player);

	/// <summary>
	/// 攻撃中断
	/// </summary>
	void Interrupt();

	/// <summary>
	/// 軌跡エフェクト更新
	/// </summary>
	void UpdateTrailEffect(Enemy* enemy);

	/// <summary>
	/// 軌跡エフェクト描画
	/// </summary>
	void DrawTrailEffect();

public:
	// Getter
	bool IsComplete() const { return isComplete_; }
	Phase GetPhase() const { return phase_; }
	uint32_t GetChargeCount() const { return chargeCount_; }
	uint32_t GetMaxChargeCount() const { return maxChargeCount_; }

	// Setter
	void SetMaxChargeCount(uint32_t count) { maxChargeCount_ = count; }

private:
	/// <summary>
	/// 予備動作更新
	/// </summary>
	void UpdatePreparation(Enemy* enemy);

	/// <summary>
	/// 突進更新
	/// </summary>
	void UpdateCharging(Enemy* enemy, Player* player);

	/// <summary>
	/// 回復動作更新
	/// </summary>
	void UpdateRecovery(Enemy* enemy);

	/// <summary>
	/// 突進中の当たり判定チェック
	/// </summary>
	void CheckCollision(Player* player);

private:
	// フェーズ
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	// 突進方向と開始位置
	Vector3 chargeDirection_ = { 0.0f, 0.0f, 1.0f };
	Vector3 chargeStartPos_ = { 0.0f, 0.0f, 0.0f };

	// タイマー
	uint32_t preparationTimer_ = 0;
	uint32_t chargingTimer_ = 0;
	uint32_t recoveryTimer_ = 0;

	// 突進回数
	uint32_t chargeCount_ = 0;
	uint32_t maxChargeCount_ = 1;

	// 元の回転
	Vector3 originalRotation_ = { 0.0f, 0.0f, 0.0f };

	// 定数
	static constexpr uint32_t kPreparationTime_ = 60;		// 予備動作時間
	static constexpr uint32_t kChargingTime_ = 60;			// 突進時間
	static constexpr uint32_t kRecoveryTime_ = 20;			// 回復時間
	static constexpr uint32_t kNextChargeDelay_ = 30;		// 次の突進までの待機時間
	static constexpr float kChargeSpeed_ = 0.3f;			// 突進速度
	static constexpr float kPreparationTiltAngle_ = 0.5f;	// 予備動作の傾き角度
	static constexpr float kMeleeHitRadius_ = 1.5f;		// 突進の当たり判定半径
	static constexpr int kMeleeDamage_ = 100;			// 突進ダメージ
	bool hitRegistered_ = false;						// 1回の突進あたりのダメージ多重ヒット防止フラグ

	// 軌跡パーティクル
	std::unique_ptr<ParticleEmitter> trailEffect_;
	Vector3 lastTrailPosition_ = { 0.0f, 0.0f, 0.0f };
	float trailEmitDistance_ = 0.5f;
	static constexpr float kFootOffsetY_ = -0.8f;
};