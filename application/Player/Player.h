#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

#include <Arm/PlayerArm.h>

#include <Stage/StageManager.h>
#include <ParticleEmitter.h>

class FollowCamera;

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
	void Init()override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;
	void UpdateStartEffect();

	void UpdateAttack();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);
	void DrawParticle(const ViewProjection& viewProjection);

	/// <summary>
	/// ImGui
	/// </summary>
	void ImGui();

public:

	/// <summary>
	/// 当たり判定
	/// </summary>
	/// <param name="other"></param>
	void OnCollision([[maybe_unused]] Collider* other) override;

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	Vector3 GetCenterPosition() const override { return transform_.translation_; }
	Vector3 GetCenterRotation() const override { return transform_.rotation_; }
	uint32_t GetSerialNumber() const { return serialNumber_; }
	Vector3 GetVelocity() const { return velocity_; }
	bool GetIsEnd() const { return isEnd; }
	bool IsDodging() const { return behavior_ == Behavior::kDodge; }
	GameState GetGameState() const { return gameState_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetViewProjection(const ViewProjection* viewProjection) { vp_ = viewProjection; }
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	void SetGameState(GameState state) { gameState_ = state; }

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
	/// 回避処理
	/// </summary>
	void StartDodge(const Vector3& direction);
	void UpdateDodge();

	/// <summary>
	/// 演出用処理
	/// </summary>
	void UpdateTrailEffect();

	/// <summary>
	/// 被弾処理
	/// </summary>
	void TakeDamage(const Vector3& hitPosition);
	void UpdateHitReaction();

	/// <summary>
	/// ゲームオーバー演出
	/// </summary>
	void UpdateGameOverEffect();

	/// <summary>
	/// ゲームクリア演出
	/// </summary>
	void UpdateGameClearEffect();

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

	// Behavior
	Behavior behavior_ = Behavior::kRoot;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// Move関連変数
	bool isMove_ = false;
	Vector3 velocity_{};
	float kAcceleration_ = 0.1f;
	const float kMaxSpeed_ = 0.1f;
	float kRotateAcceleration_ = 0.1f;

	// Attack関連変数
	bool isAttack_ = false;
	uint32_t globalComboCount_ = 0;
	uint32_t globalComboTimer_ = 0;

	// Dodge関連変数
	static constexpr int kDodgeDuration_ = 30;
	static constexpr float kDodgeSpeed_ = 0.3f;
	static constexpr float kDodgeTiltAngle_ = 0.52f;
	static constexpr int kDodgeTiltInDuration_ = 10;
	static constexpr int kDodgeTiltOutDuration_ = 10;
	int dodgeTimer_ = 0;
	Vector3 dodgeDirection_{};
	Vector3 dodgeStartRotation_{};
	Vector3 dodgeTiltRotation_{};

	// 被弾リアクション関連変数
	bool isHitReacting_ = false;
	int hitReactionTimer_ = 0;
	Vector3 hitShakeOffset_{};
	Vector3 originalPosition_{};
	static constexpr int kHitReactionDuration_ = 15;
	static constexpr float kHitShakeIntensity_ = 0.15f;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> hitEffect_;
	std::unique_ptr<ParticleEmitter> damageEffect_;

	// 出現演出関連変数
	bool isStart = false;
	bool isEnd = false;
	float easeT = 0.0f;

	// ゲームクリア演出関連変数
	float clearEffectTimer_ = 0.0f;
	const float kJumpCycle_ = 40.0f;
	const float kJumpHeight_ = 2.0f;
	Vector3 clearStartPos_{};

	// 軌跡パーティクル関連変数
	std::unique_ptr<ParticleEmitter> trailEffect_;
	Vector3 lastTrailPosition_{};
	float trailEmitDistance_ = 0.5f;  // パーティクルを発生させる移動距離の閾値
	bool isTrailActive_ = false;
	static constexpr float kFootOffsetY_ = -0.8f;

	// --- シリアルナンバー ---
	uint32_t serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// --- 各ポインタ ---
	FollowCamera* followCamera_ = nullptr;

	// --- ステージマネージャー ---
	StageManager* stageManager_ = nullptr;
};