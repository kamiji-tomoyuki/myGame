#pragma once
#include "Vector3.h"
#include "GlobalVariables.h"
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

	PlayerArmRush();

	/// <summary>
	/// ラッシュ開始
	/// </summary>
	/// <param name="isRightArm">右腕かどうか</param>
	/// <param name="currentTranslation">現在の腕の位置（ローカル座標）</param>
	/// <param name="timerOffset">
	///   rushTimer_ の初期値。交互パンチを実現するため、
	///   左右の腕に異なるオフセットを渡す（例：右腕=0, 左腕=kRushInterval_/2）。
	/// </param>
	void StartRush(bool isRightArm, const Vector3& currentTranslation,
		uint32_t timerOffset = 0);

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
	/// <summary>現フレームで計算された腕の回転を取得</summary>
	Vector3 GetCurrentRotation()      const { return currentRandomRotation_; }

	void SetHasFinisherHit(bool v) { hasFinisherHit_ = v; }
	void SetIsFinisherHitFrame(bool v) { isFinisherHitFrame_ = v; }
	void SetLastRushHitFrame(int frame) { lastRushHitFrame_ = frame; }
	void SetRushAttackDamage(uint32_t d) { rushAttackDamage_ = d; }
	void SetFinisherAttackDamage(uint32_t d) { finisherAttackDamage_ = d; }

	/// <summary>
	/// 交互パンチ用：連打フェーズが次のパンチを出せる状態かどうか。
	/// rushTimer_ が kRushInterval_ の倍数になるタイミングを外部から確認するために使う。
	/// </summary>
	uint32_t GetRushInterval() const { return kRushInterval_; }

#pragma endregion

private:

	void UpdateRapidPunch();
	void UpdateWindUp();
	void UpdateFinisher();
	void UpdateRecover();
	void ApplyVariables();

	Vector3 WorldOffsetToLocal(const Vector3& worldOffset, float currentBodyRotY) const;

private:

	bool      isRush_ = false;
	bool      isRightArm_ = true;
	RushPhase rushPhase_ = RushPhase::kRapidPunch;

	Vector3   originalPosition_ = {};
	Vector3   targetPosition_ = {};
	Vector3   currentTranslation_ = {};
	Vector3   attackDirection_ = { 0.0f, 0.0f, 1.0f };

	uint32_t  rushTimer_ = 0;
	uint32_t  rushAttackTimer_ = 0;
	uint32_t  rushCount_ = 0;
	uint32_t  rushPhaseTimer_ = 0;
	bool      rushAttackActive_ = false;
	int       lastRushHitFrame_ = -999;

	float     rushPhaseProgress_ = 0.0f;
	float     finisherProgress_ = 0.0f;

	bool      isFinisherHitFrame_ = false;
	bool      hasFinisherHit_ = false;

	uint32_t  rushAttackDamage_ = 20;
	uint32_t  finisherAttackDamage_ = 150;

	float     bodyRotYAtWindUpStart_ = 0.0f;
	float     currentBodyRotY_ = 0.0f;

	// -------------------------------------------------------
	// GlobalVariables で調整可能な変数（constexpr から昇格）
	// -------------------------------------------------------
	// 連打フェーズ
	uint32_t kRushDuration_ = 80;
	uint32_t kRushInterval_ = 8;
	uint32_t kRushAttackDuration_ = 12;
	// フィニッシャーフェーズ
	uint32_t kWindUpDuration_ = 22;
	uint32_t kFinisherDuration_ = 28;
	uint32_t kRecoverDuration_ = 25;
	// 腕移動量
	float kRushDistance_ = 1.5f;
	float kWindUpArmRetreat_ = -1.6f;
	float kWindUpArmSideR_ = 0.8f;
	float kFinisherArmAdvance_ = 4.0f;
	float kLArmWindUpZ_ = -0.6f;
	float kLArmFinisherZ_ = -1.0f;

	// --- 定数 ---
	/// 連続パンチの左右振り幅の基準値
	static inline const float kRapidPunchSideBase_ = 0.5f;
	/// 右腕・左腕それぞれの追加オフセット量
	static inline const float kRapidPunchArmSideOffset_ = 0.3f;
	/// 連続パンチイージングの折り返し進捗（これ以降は戻り動作）
	static inline const float kRapidPunchEasingTurnPoint_ = 0.6f;
	/// 連続パンチ戻り動作の速度係数
	static inline const float kRapidPunchReturnSpeed_ = 2.5f;
	/// フィニッシャーの前半終了進捗（振り出し → 最大伸ばし）
	static inline const float kFinisherHalfPoint_ = 0.5f;
	/// フィニッシャーの戻り量係数（最大伸ばしからわずかに引く割合）
	static inline const float kFinisherRetractFactor_ = 0.25f;
	/// フィニッシャーヒット判定の開始進捗
	static inline const float kFinisherHitStartProgress_ = 0.3f;
	/// フィニッシャーヒット判定の終了進捗
	static inline const float kFinisherHitEndProgress_ = 0.7f;
	/// リカバリー終了位置の補間割合（フィニッシャー伸ばし終端の何割の位置から戻るか）
	static inline const float kRecoverStartRatio_ = 0.75f;
	/// リカバリー開始から何フレーム後にヒット判定フレームを解除するか
	static inline const uint32_t kRecoverHitFrameClearDelay_ = 2;

	// -------------------------------------------------------
	// 揺れ・ランダム挙動用
	// -------------------------------------------------------
	Vector3 targetRandomRotation_ = { 0.0f, 0.0f, 0.0f };
	Vector3 currentRandomRotation_ = { 0.0f, 0.0f, 0.0f };
	const float kMaxRandomOffsetX_ = 0.8f;
	const float kMaxRandomOffsetY_ = 0.8f;
	const float kMaxRandomOffsetZ_ = 0.3f;

	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
};