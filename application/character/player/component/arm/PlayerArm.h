#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "PlayerArmAttack.h"
#include "PlayerArmRush.h"
#include <CollisionTypeIdDef.h>
#include <memory>
#include <unordered_map>

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

	void Init(std::string filePath);
	void Update() override;

	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	void StartAttack(AttackType attackType);
	void StartRush();
	bool CanCombo()     const;
	bool CanStartRush() const;

public:

	void OnCollisionEnter([[maybe_unused]] Collider* other) override;
	void OnCollision([[maybe_unused]] Collider* other) override;

public:
#pragma region getter setter

	int        GetID() { return serialNumber_; }
	Player* GetPlayer() { return player_; }
	bool       GetIsAttack() { return attack_->GetIsAttack(); }
	bool       GetIsRush() { return rush_->GetIsRush(); }
	AttackType GetCurrentAttackType() { return attack_->GetCurrentAttackType(); }
	AttackType GetLastAttackType() { return attack_->GetLastAttackType(); }
	Behavior   GetBehavior() { return behavior_; }
	Vector3    GetCenterPosition() const override { return GetWorldPosition(); }
	Vector3    GetCenterRotation() const override { return GetWorldRotation(); }
	Vector3    GetAttackDirection() const;
	uint32_t   GetComboCount()     const { return attack_->GetComboCount(); }

	// ★ 追加 — PlayerUltimate からローカル姿勢を読み書きするために必要
	Vector3    GetLocalRotation()    const { return transform_.rotation_; }
	Vector3    GetLocalTranslation() const { return transform_.translation_; }

	RushPhase  GetRushPhase()          const { return rush_->GetRushPhase(); }
	bool       IsFinisherPhase()       const { return rush_->IsFinisherPhase(); }
	bool       IsWindUpPhase()         const { return rush_->IsWindUpPhase(); }
	bool       IsRecoverPhase()        const { return rush_->IsRecoverPhase(); }
	bool       IsFinisherHitFrame()    const { return rush_->IsFinisherHitFrame(); }
	bool       HasFinisherHit()        const { return rush_->HasFinisherHit(); }
	void       SetFinisherHit() { rush_->SetHasFinisherHit(true); }
	float      GetFinisherProgress()   const { return rush_->GetFinisherProgress(); }
	float      GetRushPhaseProgress()  const { return rush_->GetRushPhaseProgress(); }
	bool       IsRightArm()            const { return isRightArm_; }

	void SetID(int id) { serialNumber_ = id; }
	void SetColliderID(CollisionTypeIdDef id) { Collider::SetTypeID(static_cast<uint32_t>(id)); }
	void SetPlayer(Player* player);
	void SetComboTimer(uint32_t comboT) { attack_->SetComboTimer(comboT); }
	void SetComboCount(uint32_t count) { attack_->SetComboCount(count); }
	void SetIsRightArm(bool isRight) { isRightArm_ = isRight; }

	// ★ 追加 — PlayerAttack への参照をセット（Init後に Player から呼ぶ）
	void SetPlayerAttack(PlayerAttack* pa) { playerAttack_ = pa; }

	void SetTranslation(Vector3 pos) { transform_.translation_ = pos; }
	void SetTranslationY(float pos) { transform_.translation_.y = pos; }
	void SetTranslationX(float pos) { transform_.translation_.x = pos; }
	void SetTranslationZ(float pos) { transform_.translation_.z = pos; }

	void SetRotation(Vector3 rotate) { transform_.rotation_ = rotate; }
	void SetRotationX(float rotate) { transform_.rotation_.x = rotate; }
	void SetRotationY(float rotate) { transform_.rotation_.y = rotate; }
	void SetRotationZ(float rotate) { transform_.rotation_.z = rotate; }

	void SetScale(Vector3 scale) { transform_.scale_ = scale; }
	void SetColliderSize(float size) { Collider::SetRadius(size); }
	void SetOriginalPosition(const Vector3& pos) { originalPosition_ = pos; }

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

	Vector3  originalPosition_ = {};

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