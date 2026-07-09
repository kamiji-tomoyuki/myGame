#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "GlobalVariables.h"

#include <array>
#include <vector>

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
#include "PlayerComboMotion.h"

// State Pattern
#include "IPlayerState.h"

using namespace Engine;
class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤークラス
/// State Pattern により行動・状態管理を各状態クラスへ委譲する
/// </summary>
class Player : public BaseObject
{
	// 各状態クラス・行動クラスが内部メンバに直接アクセスできるようにする
	friend class PlayerStatePlaying;
	friend class PlayerStateGameOver;
	friend class PlayerStateGameClear;
	friend class PlayerBehaviorRoot;
	friend class PlayerBehaviorDodge;
	friend class PlayerBehaviorHitReact;

public:

	enum ModelArm {
		kRArm,
		kLArm,
		kModelNum,
	};

	// ※ Behavior enum は削除 — IPlayerBehavior の派生クラスで表現する
	// ※ GameState enum は状態遷移トリガーとして残す（外部通知用）
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
	bool      IsRangedInvincible() const { return hitReaction_ && hitReaction_->IsRangedCooldownActive(); }
	GameState GetGameState()       const { return gameState_; }
	const std::array<std::unique_ptr<PlayerArm>, kModelNum>& GetArms() const { return arms_; }

	// ラッシュ連打の残像（トレール）腕を開始・描画する
	void StartRushTrails(uint32_t rushInterval, uint32_t rightBaseOffset, uint32_t leftBaseOffset);
	void DrawRushTrails(const ViewProjection& viewProjection);

	bool CanRightPunch()      const;
	bool CanLeftPunch()       const;
	bool CanRush()            const;
	bool IsRightPunchActive() const;
	bool IsLeftPunchActive()  const;
	bool IsRushActive()       const;
	bool IsUltimateActive()   const;  // ★ 追加

	// --- モーション駆動コンボ / フィニッシャー ---
	bool IsComboMotionActive() const { return comboMotion_ && comboMotion_->IsActive(); }
	bool IsFinisherActive()    const { return finisherMotion_ && finisherMotion_->IsActive(); }
	bool IsAnyMotionActive()   const { return IsComboMotionActive() || IsFinisherActive(); }
	PlayerComboMotion::Result TryComboAttack() { return comboMotion_ ? comboMotion_->TryAdvance() : PlayerComboMotion::Result::kNone; }
	// バッファ入力がコンボ最終段の受付窓に達したラッシュ要求を取り出す（true=ラッシュ開始）
	bool ConsumeComboRush() { return comboMotion_ && comboMotion_->ConsumePendingRush(); }
	void StopComboMotion() { if (comboMotion_) { comboMotion_->Stop(); } }

	// コンボ/フィニッシャー中の体ひねりを facing に加算/除去する（ロックオンへ二重に効かないよう対で呼ぶ）。
	// RemoveComboBodyTwist(ロックオン前) → ApplyComboBodyTwist(腕更新前) の順に PlayerStatePlaying から呼ぶ。
	void RemoveComboBodyTwist();
	void ApplyComboBodyTwist();

	Enemy* GetEnemy()         const { return enemy_; }  // ★ 追加

	void ApplyDamage(uint32_t damage, const Vector3& hitPosition);
	void ApplyDamageDirect(uint32_t damage, const Vector3& hitPosition);
	void StartRangedCooldown() { if (hitReaction_) { hitReaction_->StartRangedCooldown(); } }

	// =============================================================
	// セッター
	// =============================================================
	FollowCamera* GetFollowCamera() const { return followCamera_; }
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
	void MoveInternal();
	void ApplyComboMotion();   // コンボモーションの更新・腕姿勢反映・ヒット判定
	void UpdateRushFinisher(); // ラッシュ連打完了を検知しフィニッシャークリップを開始
	void ApplyFinisherMotion();// フィニッシャークリップの更新・腕姿勢反映・ヒット判定
	void ApplyMotionArmsAndHit(PlayerComboMotion* motion); // 腕姿勢適用＋距離ヒット（combo/finisher共通）
	Vector3 GetActiveMotionBodyRotate() const;             // 有効なモーションの体回転（combo優先）
	void UpdateLockOn();
	void TakeDamage(const Vector3& hitPosition);
	void ChangeState(std::unique_ptr<IPlayerState> next);
	void ApplyVariables();

private:

	// モデル
	std::unique_ptr<Object3d> obj3d_;
	const ViewProjection* vp_ = nullptr;

	// 腕
	std::array<std::unique_ptr<PlayerArm>, kModelNum> arms_;
	// ラッシュ連打の残像（トレール）腕プール。左右それぞれ最大 kMaxTrail_ 本。
	// ※各腕はスキニングgltf＝重いので過剰生成しない（多いとFPS低下・モデル多重生成）。
	static constexpr int kMaxTrail_ = 4;
	std::array<std::vector<std::unique_ptr<PlayerArm>>, kModelNum> trailArms_;

	// サブシステム
	std::unique_ptr<PlayerMove>           move_;
	std::unique_ptr<PlayerAttack>         attack_;
	std::unique_ptr<PlayerDodge>          dodge_;
	std::unique_ptr<PlayerRushPosture>    rushPosture_;
	std::unique_ptr<PlayerHitReaction>    hitReaction_;
	std::unique_ptr<PlayerComboMotion>    comboMotion_;
	std::unique_ptr<PlayerComboMotion>    finisherMotion_; // ラッシュ最後の一撃（単一クリップ）
	bool                                  rushFinisherStarted_ = false;

	std::unique_ptr<PlayerStartEffect>    startEffect_;
	std::unique_ptr<PlayerGameOverEffect> gameOverEffect_;
	std::unique_ptr<PlayerGameClearEffect> gameClearEffect_;

	// ステータス
	bool      isAlive_ = true;

	// GameState
	GameState gameState_ = GameState::kPlaying;

	// ===========================================================
	//  State Pattern — 状態オブジェクト
	// ===========================================================
	std::unique_ptr<IPlayerState> currentState_;

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

	// コンボモーションで体へ加算中のひねり角（次フレームのロックオン前に除去する）
	Vector3  comboBodyTwist_ = { 0.0f, 0.0f, 0.0f };

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

	// GlobalVariables
	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
	static const std::string kTrailGroupName_;

	// GlobalVariables で調整可能な変数
	uint32_t kMaxHP_Adjustable_ = 1000;       // 最大HP
	Vector3  kRightArmTranslation_ = { 1.7f, 0.0f, 1.3f };  // 右腕 初期位置
	Vector3  kLeftArmTranslation_ = { -1.7f, 0.0f, 1.3f };  // 左腕 初期位置
	Vector3  kArmScale_ = { 0.8f, 0.8f, 0.8f };  // 腕スケール
	float    kMotionHitRange_ = 7.0f;  // コンボ/フィニッシャーの距離ヒット半径（水平・GlobalVariablesで調整可）

	// ラッシュ残像（トレール）調整値
	int   kTrailCount_ = 3;        // 片側あたりの残像本数（0..kMaxTrail_）
	float kTrailMaxAlpha_ = 0.5f;  // 先頭残像のアルファ
	float kTrailFalloff_ = 0.6f;   // 後続残像ほどアルファを掛け減衰
	int   kTrailOffsetStep_ = 2;   // 残像ごとのラッシュタイマーオフセット差（フレーム）
};