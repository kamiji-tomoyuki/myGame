#pragma once
#include "Vector3.h"
#include <cstdint>
#include <cmath>

/// <summary>
/// プレイヤー腕 - ラッシュ攻撃ロジック
/// </summary>
class PlayerArmRush
{
public:

	/// <summary>
	/// ラッシュのフェーズ
	/// </summary>
	enum class RushPhase {
		kRapidPunch,	// 連続パンチフェーズ
		kWindUp,		// フィニッシャー振りかぶりフェーズ（体を右にひねり右腕を引く）
		kFinisher,		// フィニッシャー大パンチフェーズ（体を左にひねり右腕を振り出す）
		kRecover,		// フィニッシャー後の戻りフェーズ
	};

public:

	/// <summary>
	/// ラッシュ開始
	/// </summary>
	/// <param name="isRightArm">右腕かどうか</param>
	/// <param name="currentTranslation">現在の腕の位置（ローカル座標）</param>
	void StartRush(bool isRightArm, const Vector3& currentTranslation);

	/// <summary>
	/// 更新（毎フレーム呼ぶ）
	/// </summary>
	/// <param name="currentBodyRotY">現在のプレイヤー体のY回転（ラジアン）</param>
	/// <returns>ラッシュ全体が終了したら true</returns>
	bool Update(float currentBodyRotY);

public:
#pragma region getter

	bool       GetIsRush()             const { return isRush_; }
	RushPhase  GetRushPhase()          const { return rushPhase_; }
	float      GetFinisherProgress()   const { return finisherProgress_; }
	float      GetRushPhaseProgress()  const { return rushPhaseProgress_; }
	bool       IsFinisherPhase()       const { return rushPhase_ == RushPhase::kFinisher; }
	bool       IsWindUpPhase()         const { return rushPhase_ == RushPhase::kWindUp; }
	bool       IsRecoverPhase()        const { return rushPhase_ == RushPhase::kRecover; }
	bool       IsFinisherHitFrame()    const { return isFinisherHitFrame_; }
	bool       HasFinisherHit()        const { return hasFinisherHit_; }
	bool       IsRushAttackActive()    const { return rushAttackActive_; }
	uint32_t   GetRushTimer()          const { return rushTimer_; }
	uint32_t   GetRushAttackTimer()    const { return rushAttackTimer_; }
	int        GetLastRushHitFrame()   const { return lastRushHitFrame_; }
	uint32_t   GetRushAttackDamage()   const { return rushAttackDamage_; }
	uint32_t   GetFinisherAttackDamage() const { return finisherAttackDamage_; }
	Vector3    GetAttackDirection()    const { return attackDirection_; }

	/// <summary>現フレームで計算された腕の位置を取得</summary>
	Vector3 GetCurrentTranslation()   const { return currentTranslation_; }

	void SetHasFinisherHit(bool v) { hasFinisherHit_ = v; }
	void SetIsFinisherHitFrame(bool v) { isFinisherHitFrame_ = v; }
	void SetLastRushHitFrame(int frame) { lastRushHitFrame_ = frame; }
	void SetRushAttackDamage(uint32_t d) { rushAttackDamage_ = d; }
	void SetFinisherAttackDamage(uint32_t d) { finisherAttackDamage_ = d; }

#pragma endregion

private:

	void UpdateRapidPunch();
	void UpdateWindUp();
	void UpdateFinisher();
	void UpdateRecover();

	/// <summary>
	/// ワールド空間のオフセットを、現在の体回転を考慮したローカル座標に変換する。
	/// 体がひねられていても腕がワールド的にまっすぐ前に出るよう補正する。
	/// </summary>
	/// <param name="worldOffset">ワールド空間での移動オフセット（ラッシュ開始時の体向きを基準）</param>
	/// <param name="currentBodyRotY">現在の体のY回転（ラジアン）</param>
	/// <returns>ローカル空間でのオフセット</returns>
	Vector3 WorldOffsetToLocal(const Vector3& worldOffset, float currentBodyRotY) const;

private:

	bool      isRush_ = false;
	bool      isRightArm_ = true;
	RushPhase rushPhase_ = RushPhase::kRapidPunch;

	// 位置情報（すべてローカル座標）
	Vector3   originalPosition_ = {};
	Vector3   targetPosition_ = {};
	Vector3   currentTranslation_ = {};
	Vector3   attackDirection_ = { 0.0f, 0.0f, 1.0f };

	// タイマー類
	uint32_t  rushTimer_ = 0;
	uint32_t  rushAttackTimer_ = 0;
	uint32_t  rushCount_ = 0;
	uint32_t  rushPhaseTimer_ = 0;
	bool      rushAttackActive_ = false;
	int       lastRushHitFrame_ = -999;

	// フェーズ進行度
	float     rushPhaseProgress_ = 0.0f;
	float     finisherProgress_ = 0.0f;

	// フィニッシャー
	bool      isFinisherHitFrame_ = false;
	bool      hasFinisherHit_ = false;

	// ダメージ
	uint32_t  rushAttackDamage_ = 20;
	uint32_t  finisherAttackDamage_ = 150;

	// -------------------------------------------------------
	// フィニッシャー補正用
	//   WindUp開始時点のプレイヤー体のY回転を保存し、
	//   フィニッシャー中は毎フレームの体回転との差分でローカル座標を補正する
	// -------------------------------------------------------
	float     bodyRotYAtWindUpStart_ = 0.0f;	// WindUp開始時の体のY回転（ラジアン）
	float     currentBodyRotY_ = 0.0f;			// 毎フレーム更新される現在の体のY回転

	// -------------------------------------------------------
	// 定数
	// -------------------------------------------------------
	// 連打フェーズ
	static constexpr uint32_t kRushDuration = 80;
	static constexpr uint32_t kRushInterval = 8;
	static constexpr uint32_t kRushAttackDuration = 12;

	// フィニッシャーフェーズ
	static constexpr uint32_t kWindUpDuration = 22;
	static constexpr uint32_t kFinisherDuration = 28;
	static constexpr uint32_t kRecoverDuration = 25;

	// 腕移動量
	static constexpr float kRushDistance = 1.5f;

	// 右腕フィニッシャー移動量
	static constexpr float kWindUpArmRetreat = -1.6f;
	static constexpr float kWindUpArmSideR = 0.8f;
	static constexpr float kFinisherArmAdvance = 4.0f;

	// 左腕フィニッシャー移動量
	static constexpr float kLArmWindUpZ = -0.6f;
	static constexpr float kLArmFinisherZ = -1.0f;
};