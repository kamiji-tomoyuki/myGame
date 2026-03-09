#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "PlayerArmAttack.h"
#include "PlayerArmRush.h"
#include <CollisionTypeIdDef.h>
#include <memory>

class Player;

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
		kNormal,	// 通常
		kAttack,	// 攻撃中
		kSkill,		// スキル
		kRush,		// ラッシュ攻撃中
	};

	// 外部から使う型は Attack / Rush クラスのものをそのまま公開
	using AttackType = PlayerArmAttack::AttackType;
	using RushPhase = PlayerArmRush::RushPhase;

public:

	PlayerArm();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init(std::string filePath);

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	/// <summary>
	/// 攻撃処理
	/// </summary>
	void StartAttack(AttackType attackType);
	void StartRush();
	bool CanCombo()      const;
	bool CanStartRush()  const;

public:

	/// <summary>
	/// 当たり判定
	/// </summary>
	void OnCollision([[maybe_unused]] Collider* other) override;

public:
#pragma region getter setter

	int       GetID() { return serialNumber_; }
	Player* GetPlayer() { return player_; }
	bool      GetIsAttack() { return attack_->GetIsAttack(); }
	bool      GetIsRush() { return rush_->GetIsRush(); }
	AttackType GetCurrentAttackType() { return attack_->GetCurrentAttackType(); }
	AttackType GetLastAttackType() { return attack_->GetLastAttackType(); }
	Behavior  GetBehavior() { return behavior_; }
	Vector3   GetCenterPosition() const override { return GetWorldPosition(); }
	Vector3   GetCenterRotation() const override { return GetWorldRotation(); }
	Vector3   GetAttackDirection() const;
	uint32_t  GetComboCount()     const { return attack_->GetComboCount(); }

	// ラッシュフェーズ情報
	RushPhase GetRushPhase()          const { return rush_->GetRushPhase(); }
	bool      IsFinisherPhase()       const { return rush_->IsFinisherPhase(); }
	bool      IsWindUpPhase()         const { return rush_->IsWindUpPhase(); }
	bool      IsRecoverPhase()        const { return rush_->IsRecoverPhase(); }
	bool      IsFinisherHitFrame()    const { return rush_->IsFinisherHitFrame(); }
	float     GetFinisherProgress()   const { return rush_->GetFinisherProgress(); }
	float     GetRushPhaseProgress()  const { return rush_->GetRushPhaseProgress(); }
	bool      IsRightArm()            const { return isRightArm_; }

	void SetID(int id) { serialNumber_ = id; }
	void SetColliderID(CollisionTypeIdDef id) { Collider::SetTypeID(static_cast<uint32_t>(id)); }
	void SetPlayer(Player* player);
	void SetComboTimer(uint32_t comboT) { attack_->SetComboTimer(comboT); }
	void SetComboCount(uint32_t count) { attack_->SetComboCount(count); }
	void SetIsRightArm(bool isRight) { isRightArm_ = isRight; }

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

	// モデル
	std::unique_ptr<Object3d> obj3d_;

	// 攻撃・ラッシュのロジック
	std::unique_ptr<PlayerArmAttack> attack_;
	std::unique_ptr<PlayerArmRush>   rush_;

	// 腕の基本状態
	bool     isRightArm_ = true;
	Behavior behavior_ = Behavior::kNormal;

	// 腕の基準位置（Init時に設定、SetOriginalPosition でも更新可能）
	Vector3 originalPosition_ = {};

	// シリアルナンバー
	uint32_t          serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// ポインタ
	Player* player_ = nullptr;
};