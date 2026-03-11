#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

#include <Sprite.h>
#include <StageManager.h>
#include <ParticleEmitter.h>

#include <PlayerArm.h>
#include "PlayerStartEffect.h"
#include "PlayerGameOverEffect.h"
#include "PlayerGameClearEffect.h"
#include "PlayerAttack.h"
#include "PlayerDodge.h"
#include "PlayerHitReaction.h"
#include "PlayerMove.h"
#include "PlayerRushPosture.h"

class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤークラス
/// </summary>
class Player : public BaseObject
{
public:

	enum ModelArm {
		kRArm,
		kLArm,
		kModelNum,
	};

	enum class Behavior {
		kRoot,
		kAttack,
		kDodge,
	};

	enum class GameState {
		kPlaying,
		kGameOver,
		kGameClear,
	};

public:

	Player();

	void Init() override;
	void Update() override;
	void UpdateStartEffect();

	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawSprite(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	void ImGui();

public:

	void OnCollision([[maybe_unused]] Collider* other) override;

public:

	// =============================================================
	// ゲッター
	// =============================================================
	Vector3   GetCenterPosition()  const override { return transform_.translation_; }
	Vector3   GetCenterRotation()  const override { return transform_.rotation_; }
	uint32_t  GetSerialNumber()    const { return serialNumber_; }
	Vector3   GetVelocity()        const;
	bool      GetIsEnd()           const { return startEffect_ && startEffect_->IsEnd(); }
	bool      IsDodging()          const { return dodge_ && dodge_->IsDodging(); }
	bool      IsHitReacting()      const { return hitReaction_ && hitReaction_->IsHitReacting(); }
	GameState GetGameState()       const { return gameState_; }
	const std::array<std::unique_ptr<PlayerArm>, kModelNum>& GetArms() const { return arms_; }

	bool CanRightPunch()      const;
	bool CanLeftPunch()       const;
	bool CanRush()            const;
	bool IsRightPunchActive() const;
	bool IsLeftPunchActive()  const;
	bool IsRushActive()       const;

	void ApplyDamage(uint32_t damage, const Vector3& hitPosition);

	// =============================================================
	// セッター
	// =============================================================
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetViewProjection(const ViewProjection* vp) { vp_ = vp; }
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& t) { transform_.translation_ = t; }
	void SetVelocity(const Vector3& v);
	void SetGameState(GameState state) { gameState_ = state; }
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }

	void UpdateArms();
	void UpdateModelAnimation();

private:

	void InitArm();
	void Move();
	void UpdateLockOn();
	void TakeDamage(const Vector3& hitPosition);

private:

	// モデル
	std::unique_ptr<Object3d> obj3d_;
	const ViewProjection* vp_ = nullptr;

	// 腕
	std::array<std::unique_ptr<PlayerArm>, kModelNum> arms_;

	// サブシステム
	std::unique_ptr<PlayerMove>           move_;
	std::unique_ptr<PlayerAttack>         attack_;
	std::unique_ptr<PlayerDodge>          dodge_;
	std::unique_ptr<PlayerRushPosture>    rushPosture_;
	std::unique_ptr<PlayerHitReaction>    hitReaction_;

	std::unique_ptr<PlayerStartEffect>    startEffect_;
	std::unique_ptr<PlayerGameOverEffect> gameOverEffect_;
	std::unique_ptr<PlayerGameClearEffect> gameClearEffect_;

	// ステータス
	bool      isAlive_ = true;
	GameState gameState_ = GameState::kPlaying;
	Behavior  behavior_ = Behavior::kRoot;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// HPバー
	std::unique_ptr<Sprite> hpBarBg_;
	std::unique_ptr<Sprite> hpBar_;
	static constexpr float kHpBarFullWidth_ = 350.0f;
	static constexpr float kHpBarHeight_ = 40.0f;
	static constexpr float kHpBarBgPadding_ = 4.0f;
	Vector3 hpColor_ = { 0.0f, 1.0f, 0.0f };

	// 攻撃関連（グローバルコンボ）
	bool     isAttack_ = false;
	uint32_t globalComboCount_ = 0;
	uint32_t globalComboTimer_ = 0;

	// ロックオン
	Enemy* enemy_ = nullptr;

	// エフェクト
	std::unique_ptr<ParticleEmitter> hitEffect_;
	std::unique_ptr<ParticleEmitter> damageEffect_;
	std::unique_ptr<ParticleEmitter> trailEffect_;

	// シリアルナンバー
	uint32_t          serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// ポインタ
	FollowCamera* followCamera_ = nullptr;
	StageManager* stageManager_ = nullptr;
};