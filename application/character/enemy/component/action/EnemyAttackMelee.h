#pragma once
#include "Vector3.h"
#include "GlobalVariables.h"
#include <memory>

using namespace Engine;
class Enemy;
class Player;
namespace Engine { class ParticleEmitter; }

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
	void Interrupt(Enemy* enemy);

public:
	// Getter
	bool IsComplete() const { return isComplete_; }
	Phase GetPhase() const { return phase_; }
	uint32_t GetChargeCount() const { return chargeCount_; }
	uint32_t GetMaxChargeCount() const { return maxChargeCount_; }

	// Setter
	void SetMaxChargeCount(uint32_t count) { maxChargeCount_ = count; }

private:
	void UpdatePreparation(Enemy* enemy);
	void UpdateCharging(Enemy* enemy, Player* player);
	void UpdateRecovery(Enemy* enemy);
	void CheckCollision(Player* player);
	void ApplyVariables();

private:
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	Vector3 chargeDirection_ = { 0.0f, 0.0f, 1.0f };
	Vector3 chargeStartPos_ = { 0.0f, 0.0f, 0.0f };

	uint32_t preparationTimer_ = 0;
	uint32_t chargingTimer_ = 0;
	uint32_t recoveryTimer_ = 0;

	uint32_t chargeCount_ = 0;
	uint32_t maxChargeCount_ = 1;

	Vector3 originalRotation_ = { 0.0f, 0.0f, 0.0f };

	// -------------------------------------------------------
	// GlobalVariables で調整可能な変数（constexpr から昇格）
	// -------------------------------------------------------
	uint32_t kPreparationTime_ = 60;    // 予備動作時間
	uint32_t kChargingTime_ = 60;    // 突進時間
	uint32_t kRecoveryTime_ = 20;    // 回復時間
	uint32_t kNextChargeDelay_ = 30;    // 次の突進までの待機時間
	float    kChargeSpeed_ = 0.3f;  // 突進速度
	float    kPreparationTiltAngle_ = 0.5f;  // 予備動作の傾き角度
	float    kMeleeHitRadius_ = 1.5f;  // 突進の当たり判定半径
	int32_t  kMeleeDamage_ = 100;   // 突進ダメージ

	bool hitRegistered_ = false;

	// 突進中のY座標固定（地面に張り付かせる）
	float groundY_ = 0.0f;

	const float kFootOffsetY_ = -0.8f;

	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
};