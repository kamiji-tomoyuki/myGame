#pragma once
#include "Vector3.h"
#include <memory>
#include <unordered_map>

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
	EnemyAttackMelee* GetMeleeAttack()  const { return meleeAttack_.get(); }
	EnemyAttackRanged* GetRangedAttack() const { return rangedAttack_.get(); }
	bool IsAttacking() const { return currentAttackType_ != AttackType::kNone; }

private:

	// =============================================================
	//  AttackType ごとの更新・完了チェック関数
	// =============================================================
	using UpdateFunc = bool (EnemyAttackManager::*)(Enemy*, Player*);
	using CompleteFunc = bool (EnemyAttackManager::*)() const;

	bool UpdateMelee(Enemy* enemy, Player* player);
	bool UpdateRanged(Enemy* enemy, Player* player);

	bool IsCompleteMelee() const;
	bool IsCompleteRanged() const;

	static const std::unordered_map<AttackType, UpdateFunc>   kUpdateTable_;
	static const std::unordered_map<AttackType, CompleteFunc> kCompleteTable_;

	// 攻撃タイプ
	AttackType currentAttackType_ = AttackType::kNone;

	// 各攻撃クラス
	std::unique_ptr<EnemyAttackMelee>  meleeAttack_;
	std::unique_ptr<EnemyAttackRanged> rangedAttack_;

	// 攻撃準備タイマー
	uint32_t attackPreparationTimer_ = 0;
	const uint32_t kAttackPreparationTime_ = 180;

	// 攻撃範囲の閾値
	const float kMeleeAttackRange_ = 17.0f;
};