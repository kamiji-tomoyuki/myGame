#pragma once
#include "Vector3.h"
#include <cstdint>

/// <summary>
/// プレイヤー腕 - 通常攻撃ロジック
/// </summary>
class PlayerArmAttack
{
public:

	/// <summary>
	/// 攻撃の種類
	/// </summary>
	enum class AttackType {
		kNone,			// 攻撃なし
		kRightPunch,	// 右パンチ
		kLeftPunch,		// 左パンチ
		kRush,			// ラッシュ攻撃
	};

public:

	/// <summary>
	/// 攻撃開始
	/// </summary>
	/// <param name="attackType">攻撃の種類</param>
	/// <param name="isRightArm">右腕かどうか</param>
	/// <param name="currentTranslation">現在の腕の位置</param>
	void StartAttack(AttackType attackType, bool isRightArm, const Vector3& currentTranslation);

	/// <summary>
	/// 更新（毎フレーム呼ぶ）
	/// </summary>
	/// <returns>攻撃が終了したら true</returns>
	bool Update();

	/// <summary>
	/// コンボウィンドウの更新（毎フレーム呼ぶ）
	/// </summary>
	void UpdateComboTimer();

	/// <summary>
	/// コンボ入力が受け付けられる状態か
	/// </summary>
	bool CanCombo() const { return comboTimer_ > 0; }

	/// <summary>
	/// ラッシュ開始条件を満たしているか
	/// </summary>
	bool CanStartRush() const {
		return lastAttackType_ == AttackType::kLeftPunch && comboTimer_ > 0;
	}

public:
#pragma region getter

	bool        GetIsAttack()          const { return isAttack_; }
	AttackType  GetCurrentAttackType() const { return currentAttackType_; }
	AttackType  GetLastAttackType()    const { return lastAttackType_; }
	Vector3     GetAttackDirection()   const { return attackDirection_; }
	float       GetAttackProgress()    const { return attackProgress_; }
	uint32_t    GetComboCount()        const { return comboCount_; }
	uint32_t    GetComboTimer()        const { return comboTimer_; }
	uint32_t    GetAttackDamage()      const { return attackDamage_; }

	/// <summary>現フレームで計算された腕の位置を取得</summary>
	Vector3 GetCurrentTranslation()   const { return currentTranslation_; }

	void SetComboTimer(uint32_t t) { comboTimer_ = t; }
	void SetComboCount(uint32_t c) { comboCount_ = c; }
	void SetLastAttackType(AttackType t) { lastAttackType_ = t; }
	void SetAttackDamage(uint32_t d) { attackDamage_ = d; }
	bool GetHasHitThisAttack()       const { return hasHitThisAttack_; }
	void SetHasHitThisAttack(bool v) { hasHitThisAttack_ = v; }

#pragma endregion

private:

	bool        isAttack_ = false;
	bool        hasHitThisAttack_ = false;	// 1回の攻撃で1度だけヒットさせるフラグ
	AttackType  currentAttackType_ = AttackType::kNone;
	AttackType  lastAttackType_ = AttackType::kNone;
	Vector3     attackDirection_ = { 0.0f, 0.0f, 1.0f };
	Vector3     originalPosition_ = {};
	Vector3     targetPosition_ = {};
	Vector3     currentTranslation_ = {};
	float       attackProgress_ = 0.0f;
	uint32_t    attackTimer_ = 0;
	uint32_t    comboTimer_ = 0;
	uint32_t    comboCount_ = 0;
	uint32_t    attackDamage_ = 50;

	// -------------------------------------------------------
	// 定数
	// -------------------------------------------------------
	static constexpr uint32_t kAttackDuration = 20;
	static constexpr uint32_t kComboWindow = 30;
	static constexpr float    kAttackDistance = 2.0f;
	static constexpr float    kRightPunchOffset = -0.5f;
	static constexpr float    kLeftPunchOffset = 0.5f;
};