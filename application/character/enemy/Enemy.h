#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "GlobalVariables.h"
#include <Sprite.h>

// コンポーネント
#include "EnemyMove.h"
#include "EnemyHitReaction.h"

// エフェクト
#include "EnemyEffect.h"

// State Pattern
#include "IEnemyState.h"

class Player;
class EnemyAttackManager;

/// <summary>
/// 敵クラス
/// State Pattern により状態管理を各状態クラスへ委譲する
/// </summary>
class Enemy : public BaseObject
{
	// 各状態クラス・コンポーネントが内部メンバに直接アクセスできるようにする
	friend class EnemyStatePlaying;
	friend class EnemyStateGameOver;
	friend class EnemyStateGameClear;

public:

	/// <summary>
	/// ゲーム状態（外部通知・遷移トリガー用）
	/// </summary>
	enum class GameState {
		kPlaying,
		kGameOver,
		kGameClear,
	};

public:

	Enemy();

	void Init() override;
	void Update(Player* player, const ViewProjection& vp);
	void UpdateStartEffect();

	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);
	void DrawSprite(const ViewProjection& viewProjection);

	void ImGui();

public:

	void OnCollision([[maybe_unused]] Collider* other) override;
	void OnRushHit(bool isFinalHit);

	void HandleCollisionWithPlayer(Player* player);
	void TakeDamage(uint32_t damage);

	/// <summary>状態を切り替える</summary>
	void ChangeState(IEnemyState* next);

public:

	// --- ゲッター ---
	Vector3          GetCenterPosition()  const override { return transform_.translation_; }
	Vector3          GetCenterRotation()  const override { return transform_.rotation_; }
	uint32_t         GetSerialNumber()    const { return serialNumber_; }
	static uint32_t  GetNextSerialNumber() { return nextSerialNumber_; }
	uint32_t         GetHP()              const { return HP_; }
	bool             GetIsAlive()         const { return isAlive_; }
	GameState        GetGameState()       const { return gameState_; }
	Vector3          GetVelocity()        const { return move_ ? move_->GetVelocity() : Vector3{}; }
	bool             IsAttacking()        const;
	bool             IsRushActive()       const { return false; } // 敵はラッシュを行わない

	EnemyAttackManager* GetAttackManager() const { return attackManager_.get(); }
	EnemyHitReaction* GetHitReaction()   const { return hitReaction_.get(); }
	EnemyMove* GetMove()          const { return move_.get(); }
	Player* GetPlayer()        const { return player_; }
	Vector3             GetObjRotation()   const;
	Vector3             GetOriginalRotation() const { return originalRotation_; }

	// --- セッター ---
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& t) { transform_.translation_ = t; }
	void SetIsStart(bool isStart) { isStart_ = isStart; }
	void SetGameState(GameState state) { gameState_ = state; }
	void SetVelocity(const Vector3& v) { if (move_) { move_->SetVelocity(v); } }
	void SetIsAlive(bool alive) { isAlive_ = alive; }
	void SetObjRotation(const Vector3& rot);
	void UpdateBaseObject() { BaseObject::Update(); }

	// BaseObject ラッパー（State・コンポーネントクラスから使用）
	void    SetWorldPosition(const Vector3& pos) { BaseObject::SetWorldPosition(pos); }
	void    SetRotation(const Vector3& rot) { BaseObject::SetRotation(rot); }
	void    SetScale(const Vector3& scale) { BaseObject::SetScale(scale); }
	Vector3 GetWorldRotation()   const { return BaseObject::GetWorldRotation(); }
	Vector3 GetWorldSize()       const { return BaseObject::GetWorldSize(); }

private:

	// --- 参照 ---
	Player* player_ = nullptr;
	const ViewProjection* vp_ = nullptr;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	// --- 攻撃管理 ---
	std::unique_ptr<EnemyAttackManager> attackManager_;

	// --- コンポーネント ---
	std::unique_ptr<EnemyMove>        move_;
	std::unique_ptr<EnemyHitReaction> hitReaction_;

	// --- エフェクト ---
	std::unique_ptr<EnemyEffect> effect_;

	// --- State Pattern ---
	std::unique_ptr<IEnemyState> currentState_;

	// --- ステータス ---
	bool      isAlive_ = true;
	GameState gameState_ = GameState::kPlaying;

	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	bool isStart_ = false;

	// 元の回転（ノックバック後の戻し基準）
	Vector3 originalRotation_;

	// --- HPバー ---
	std::unique_ptr<Sprite> hpBarBg_;
	std::unique_ptr<Sprite> hpBar_;
	static constexpr float kHpBarFullWidth_ = 350.0f;
	static constexpr float kHpBarHeight_ = 40.0f;
	static constexpr float kHpBarBgPadding_ = 4.0f;

	// --- シリアルナンバー ---
	uint32_t        serialNumber_ = 0;
	static uint32_t nextSerialNumber_;

	// --- GlobalVariables ---
	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;

	// GlobalVariables で調整可能な変数
	uint32_t kMaxHP_Adjustable_ = 1000; // 最大HP（調整用）

	void ApplyVariables();

	// --- 初期化パラメータ ---
	const Vector3 kInitialWorldPosition_ = { 0.0f, 2.0f, 15.0f };
	const float   kColliderSize_ = 1.0f;
	const Vector3 kModelInitialRotation_ = { 0.0f, 1.57f * 2.0f, 0.0f };

	// --- HPバー UI座標 ---
	const float kHpBarPosX_ = 1240.0f;
	const float kHpBarPosY_ = 150.0f;
	const Vector3 kHpBarBgColor_ = { 0.2f, 0.2f, 0.2f };

	// --- 衝突処理パラメータ ---
	const float kAttackOverlapMultiplier_ = 2.0f;   // 攻撃中の重なり倍率
	const float kCollisionVelocityDamping_ = 0.1f;   // 衝突時の速度減衰係数
};