#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "PlayerArmAttack.h"
#include "PlayerArmRush.h"
#include <CollisionTypeIdDef.h>
#include <memory>
#include <unordered_map>

using namespace Engine;
class Player;
class PlayerAttack;   // 前方宣言

/// <summary>
/// プレイヤー(腕)クラス
/// </summary>
class PlayerArm : public BaseObject
{
public:

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kNormal,
		kAttack,
		kSkill,
		kRush,
	};

	using AttackType = PlayerArmAttack::AttackType;
	using RushPhase = PlayerArmRush::RushPhase;

public:

	PlayerArm();

	void Init(const std::string& filePath);
	void Update() override;

	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	void StartAttack(AttackType attackType);

	/// <summary>
	/// ラッシュ開始
	/// </summary>
	/// <param name="timerOffset">
	///   連打フェーズのタイマー初期値。
	///   左右の腕に kRushInterval/2 フレームのズレを与えることで交互パンチを実現する。
	///   デフォルト=0（右腕）。
	/// </param>
	void StartRush(uint32_t timerOffset = 0);

	bool CanCombo()     const;
	bool CanStartRush() const;

public:

	void OnCollisionEnter([[maybe_unused]] Collider* other) override;
	void OnCollision([[maybe_unused]] Collider* other) override;

public:
#pragma region getter setter

	// --- 状態の読み取り（副作用なし＝すべて const） ---
	bool       GetIsRush()             const { return rush_->GetIsRush(); }
	bool       IsRapidPunchDone()      const { return rush_->IsRapidPunchDone(); }
	AttackType GetCurrentAttackType()  const { return attack_->GetCurrentAttackType(); }
	AttackType GetLastAttackType()     const { return attack_->GetLastAttackType(); }
	Behavior   GetBehavior()           const { return behavior_; }
	Vector3    GetCenterPosition()     const override { return GetWorldPosition(); }
	Vector3    GetCenterRotation()     const override { return GetWorldRotation(); }

	// PlayerUltimate がローカル姿勢を退避/復元するために参照
	Vector3    GetLocalRotation()      const { return transform_.rotation_; }
	Vector3    GetLocalTranslation()   const { return transform_.translation_; }

	// ラッシュ姿勢制御（PlayerRushPosture 等が参照）
	RushPhase  GetRushPhase()          const { return rush_->GetRushPhase(); }
	bool       IsFinisherPhase()       const { return rush_->IsFinisherPhase(); }
	float      GetFinisherProgress()   const { return rush_->GetFinisherProgress(); }
	float      GetRushPhaseProgress()  const { return rush_->GetRushPhaseProgress(); }
	/// <summary>この腕の PlayerArmRush が持つ連打間隔を返す（交互タイミング計算用）</summary>
	uint32_t   GetRushInterval()       const { return rush_->GetRushInterval(); }
	/// <summary>連打完了フラグを消費（フィニッシャー起動を1ラッシュ1回に限定する）</summary>
	void       ClearRapidPunchDone() { rush_->ClearRapidPunchDone(); }
	/// <summary>ラッシュの溜め(ウィンドアップ)中か（体の後傾判定用）</summary>
	bool       IsRushCharging()        const { return rush_->IsCharging(); }

	// --- 生成時のセットアップ（Player::InitArm から呼ぶ） ---
	void SetID(int id) { serialNumber_ = id; }
	void SetColliderID(CollisionTypeIdDef id) { Collider::SetTypeID(static_cast<uint32_t>(id)); }
	void SetPlayer(Player* player);
	void SetIsRightArm(bool isRight) { isRightArm_ = isRight; }
	void SetPlayerAttack(PlayerAttack* pa) { playerAttack_ = pa; }

	// --- モーション適用（コンボ/フィニッシャーが腕のローカル姿勢を上書きする） ---
	void SetTranslation(const Vector3& pos) { transform_.translation_ = pos; }
	void SetRotation(const Vector3& rotate) { transform_.rotation_ = rotate; }
	void SetScale(const Vector3& scale) { transform_.scale_ = scale; }

#pragma endregion

private:

	void HandleHit(Collider* other);

	bool UpdateAttackBehavior();
	bool UpdateRushBehavior();

	using BehaviorFunc = bool (PlayerArm::*)();
	static const std::unordered_map<Behavior, BehaviorFunc> kBehaviorTable_;

	std::unique_ptr<Object3d>        obj3d_;
	std::unique_ptr<PlayerArmAttack> attack_;
	std::unique_ptr<PlayerArmRush>   rush_;

	bool     isRightArm_ = true;
	Behavior behavior_ = Behavior::kNormal;

	uint32_t          serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	Player* player_ = nullptr;
	PlayerAttack* playerAttack_ = nullptr;

	// --- 定数 ---
	/// 初期攻撃ダメージ
	static inline const uint32_t kInitAttackDamage_ = 50;
	/// 初期ラッシュ攻撃ダメージ
	static inline const uint32_t kInitRushAttackDamage_ = 20;
	/// 初期フィニッシャー攻撃ダメージ
	static inline const uint32_t kInitFinisherAttackDamage_ = 150;
	/// コライダー半径
	static inline const float    kColliderRadius_ = 0.8f;
	/// 連続パンチのヒット判定を受け付けるタイマーの最小値
	static inline const uint32_t kRushHitTimerMin_ = 2;
	/// 連続パンチのヒット判定を受け付けるタイマーの最大値
	static inline const uint32_t kRushHitTimerMax_ = 6;
	/// 連続パンチの同一敵へのヒット間隔（フレーム）
	static inline const int      kRushHitInterval_ = 3;
	/// 通常攻撃のヒット判定を受け付ける進捗の最小値
	static inline const float    kAttackHitProgressMin_ = 0.4f;
	/// 通常攻撃のヒット判定を受け付ける進捗の最大値
	static inline const float    kAttackHitProgressMax_ = 0.6f;
};