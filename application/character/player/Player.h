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

class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤークラス
/// </summary>
class Player : public BaseObject
{
public:
	/// <summary>
	/// 連動するモデル
	/// </summary>
	enum ModelArm {
		kRArm,
		kLArm,
		kModelNum,
	};

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kRoot,		// 通常
		kAttack,	// 攻撃
		kDodge,		// 回避
	};

	/// <summary>
	/// ゲーム状態
	/// </summary>
	enum class GameState {
		kPlaying,		// ゲームプレイ中
		kGameOver,		// ゲームオーバー
		kGameClear,		// ゲームクリア
	};

public:

	Player();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 開始演出の更新（ゲーム開始ロード画面から呼ぶ）
	/// </summary>
	void UpdateStartEffect();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawSprite(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	/// <summary>
	/// ImGui
	/// </summary>
	void ImGui();

public:

	/// <summary>
	/// 当たり判定
	/// </summary>
	void OnCollision([[maybe_unused]] Collider* other) override;

public:

	// =============================================================
	// ゲッター
	// =============================================================
	Vector3  GetCenterPosition()  const override { return transform_.translation_; }
	Vector3  GetCenterRotation()  const override { return transform_.rotation_; }
	uint32_t GetSerialNumber()    const { return serialNumber_; }
	Vector3  GetVelocity()        const { return velocity_; }
	bool     GetIsEnd()           const { return startEffect_ && startEffect_->IsEnd(); }
	bool     IsDodging()          const { return dodge_ && dodge_->IsDodging(); }
	bool     IsHitReacting()      const { return isHitReacting_; }
	GameState GetGameState()      const { return gameState_; }
	const std::array<std::unique_ptr<PlayerArm>, kModelNum>& GetArms() const { return arms_; }

	/// UI表示判定（サブクラスに委譲）
	bool CanRightPunch() const;
	bool CanLeftPunch() const;
	bool CanRush()      const;

	/// 攻撃実行中判定（サブクラスに委譲）
	bool IsRightPunchActive() const;
	bool IsLeftPunchActive()  const;
	bool IsRushActive()       const;

	/// <summary>
	/// 外部からのダメージ受け付け（遠距離攻撃など）
	/// </summary>
	void ApplyDamage(uint32_t damage, const Vector3& hitPosition);

	// =============================================================
	// セッター
	// =============================================================
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetViewProjection(const ViewProjection* viewProjection) { vp_ = viewProjection; }
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	void SetGameState(GameState state) { gameState_ = state; }
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }

	/// 腕を全て更新
	void UpdateArms();

	/// アニメーション更新
	void UpdateModelAnimation();


private:

	/// <summary>
	/// 腕の初期化
	/// </summary>
	void InitArm();

	/// <summary>
	/// 移動
	/// </summary>
	void Move();

	/// <summary>
	/// ロックオン処理
	/// </summary>
	void UpdateLockOn();

	/// <summary>
	/// 演出用処理
	/// </summary>
	void UpdateTrailEffect();

	/// <summary>
	/// 被弾処理
	/// </summary>
	void TakeDamage(const Vector3& hitPosition);
	void UpdateHitReaction();

private:

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 腕 ---
	std::array<std::unique_ptr<PlayerArm>, kModelNum> arms_;

	// --- 各ステータス ---
	bool isAlive_ = true;

	// ゲーム状態
	GameState gameState_ = GameState::kPlaying;

	// Behavior（回避中は dodge_->IsDodging() で判定するが、
	//           他箇所との整合のためにも維持）
	Behavior behavior_ = Behavior::kRoot;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// HPバー（背景・本体）
	std::unique_ptr<Sprite> hpBarBg_;   // ← 追加: HPバー背景
	std::unique_ptr<Sprite> hpBar_;
	static constexpr float  kHpBarFullWidth_ = 350.0f;
	static constexpr float  kHpBarHeight_ = 40.0f;
	static constexpr float  kHpBarBgPadding_ = 4.0f;   // ← 追加: 背景の余白
	Vector3 hpColor_ = { 0.0f, 1.0f, 0.0f };

	// Move関連変数
	bool    isMove_ = false;
	Vector3 velocity_{};
	float   kAcceleration_ = 0.1f;
	const float kMaxSpeed_ = 0.1f;
	float   kRotateAcceleration_ = 0.1f;

	// Attack関連
	bool     isAttack_ = false;
	uint32_t globalComboCount_ = 0;
	uint32_t globalComboTimer_ = 0;

	// 被弾リアクション関連変数
	bool    isHitReacting_ = false;
	int     hitReactionTimer_ = 0;
	Vector3 hitShakeOffset_{};
	Vector3 originalPosition_{};
	static constexpr int   kHitReactionDuration_ = 15;
	static constexpr float kHitShakeIntensity_ = 0.15f;

	// 接触ダメージのクールダウン
	int contactDamageCooldown_ = 0;
	static constexpr int kContactDamageCooldownDuration_ = 90;

	// ロックオン関連変数
	bool    isLockOn_ = false;
	Enemy* enemy_ = nullptr;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> hitEffect_;
	std::unique_ptr<ParticleEmitter> damageEffect_;

	// 軌跡パーティクル関連変数
	std::unique_ptr<ParticleEmitter> trailEffect_;
	Vector3 lastTrailPosition_{};
	float   trailEmitDistance_ = 0.5f;
	bool    isTrailActive_ = false;
	static constexpr float kFootOffsetY_ = -0.8f;

	std::unique_ptr<PlayerStartEffect>    startEffect_;
	std::unique_ptr<PlayerGameOverEffect> gameOverEffect_;
	std::unique_ptr<PlayerGameClearEffect> gameClearEffect_;
	std::unique_ptr<PlayerAttack>         attack_;
	std::unique_ptr<PlayerDodge>          dodge_;

	// --- シリアルナンバー ---
	uint32_t          serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// --- 各ポインタ ---
	FollowCamera* followCamera_ = nullptr;

	// --- ステージマネージャー ---
	StageManager* stageManager_ = nullptr;
};