#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include "GlobalVariables.h"
#include <memory>

class Enemy;
class Player;
class ViewProjection;

/// <summary>
/// 敵の円形範囲攻撃クラス
/// </summary>
class EnemyAttackCircle
{
public:
	enum class Phase {
		kNone,
		kPreparation,	// 予備動作 (前に傾いてシェイク)
		kJump,			// 跳躍 (近距離はその場、遠距離はターゲットへ)
		kRecovery,		// 硬直
	};

public:
	EnemyAttackCircle();
	~EnemyAttackCircle();

	void Initialize();
	void Start(Enemy* enemy, Player* player);
	void Update(Enemy* enemy, Player* player);
	void UpdateViewProjection(const ViewProjection& vp);
	void Interrupt();
	void Draw(const ViewProjection& viewProjection);

	bool IsComplete() const { return isComplete_; }

private:
	void UpdatePreparation(Enemy* enemy, Player* player);
	void UpdateJump(Enemy* enemy, Player* player);
	void UpdateRecovery(Enemy* enemy);
	void ApplyVariables();
	void CheckCollision(Enemy* enemy, Player* player);

private:
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	uint32_t timer_ = 0;
	Vector3 targetPosition_;
	Vector3 currentShake_;	// 現在のシェイク値
	bool isLongRange_ = false;

	float jumpVelocityY_ = 0.0f;
	Vector3 startPosition_;
	Vector3 baseRotation_;

	// 警告表示用
	std::unique_ptr<Object3d> warningOutline_;
	std::unique_ptr<Object3d> warningFill_;
	WorldTransform warningTransform_;
	WorldTransform warningFillTransform_;
	bool isWarningActive_ = false;

	const ViewProjection* vp_ = nullptr;
	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;

	// 調整可能パラメータ
	uint32_t kPrepTime_ = 60;		// 予備動作時間
	uint32_t kRecoveryTime_ = 45;	// 着地後硬直
	float kAttackRadius_ = 15.0f;	// 攻撃半径
	float kJumpHeight_ = 15.0f;		// ジャンプの高さ (遠距離時)
	float kGravity_ = 0.05f;		// 重力
	int32_t kDamage_ = 25;			// ダメージ
	float kTiltAngle_ = 0.4f;		// 前傾角度
	float kShakeAmount_ = 0.5f;		// シェイク幅
};
