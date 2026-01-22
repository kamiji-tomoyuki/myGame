#pragma once
#include "Vector3.h"
#include <memory>

class Player;
class Enemy;
class EnemyAttackMelee;
class EnemyAttackRanged;

class EnemyAttackManager
{
public:
	enum class AttackType {
		kNone,
		kMelee,		// 近接攻撃(突進)
		kRanged,	// 遠距離攻撃
	};

public:
	EnemyAttackManager();
	~EnemyAttackManager();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Enemy* enemy, Player* player);

	/// <summary>
	/// 攻撃選択と開始
	/// </summary>
	void SelectAndStartAttack(Enemy* enemy, Player* player);

	/// <summary>
	/// 現在の攻撃が完了したかチェック
	/// </summary>
	bool IsAttackComplete() const;

	/// <summary>
	/// 攻撃をリセット
	/// </summary>
	void ResetAttack();

	/// <summary>
	/// ラッシュ攻撃による中断処理
	/// </summary>
	void InterruptByRush();

public:
	// Getter
	AttackType GetCurrentAttackType() const { return currentAttackType_; }
	EnemyAttackMelee* GetMeleeAttack() const { return meleeAttack_.get(); }
	EnemyAttackRanged* GetRangedAttack() const { return rangedAttack_.get(); }
	bool IsAttacking() const { return currentAttackType_ != AttackType::kNone; }

private:
	// 攻撃タイプ
	AttackType currentAttackType_ = AttackType::kNone;

	// 各攻撃クラス
	std::unique_ptr<EnemyAttackMelee> meleeAttack_;
	std::unique_ptr<EnemyAttackRanged> rangedAttack_;

	// 攻撃準備タイマー
	uint32_t attackPreparationTimer_ = 0;
	static constexpr uint32_t kAttackPreparationTime_ = 180;

	// 攻撃範囲の閾値
	static constexpr float kMeleeAttackRange_ = 17.0f;
};