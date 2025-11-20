#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

class Player;

class Enemy : public BaseObject
{
public:

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kRoot,		// 通常 (接近)
		kAttack,	// 攻撃 (突進)
		kCooldown,	// クールダウン
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

	Enemy();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Player* player, const ViewProjection& vp);
	void UpdateStartEffect();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection) override;
	void DrawAnimation(const ViewProjection& viewProjection);

public:

	/// <summary>
	/// 当たり判定
	/// </summary>
	/// <param name="other"></param>
	void OnCollision([[maybe_unused]] Collider* other) override;

	/// <summary>
	/// プレイヤーとの衝突処理
	/// </summary>
	void HandleCollisionWithPlayer(Player* player);

	void TakeDamage(uint32_t damage);

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	Vector3 GetCenterPosition() const override { return transform_.translation_; }
	Vector3 GetCenterRotation() const override { return transform_.rotation_; }
	uint32_t GetSerialNumber() const { return serialNumber_; }
	static uint32_t GetNextSerialNumber() { return nextSerialNumber_; }
	float GetShortDistance() { return shortDistance_; }
	uint32_t GetHP() { return HP_; }
	bool GetIsAlive() { return isAlive_; }
	Behavior GetBehavior() const { return behavior_; }
	GameState GetGameState() const { return gameState_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetIsStart(bool isStart) { isStart_ = isStart; }
	void SetGameState(GameState state) { gameState_ = state; }

private:

	/// <summary>
	/// 移動
	/// </summary>
	void Approach();

	/// <summary>
	/// 突進攻撃関連
	/// </summary>
	void UpdateBehavior();
	void StartCharge();
	void UpdateCharge();
	void EndCharge();
	void StartCooldown();
	void UpdateCooldown();

	/// <summary>
	/// ラッシュ攻撃関連処理
	/// </summary>
	void CheckPlayerRushStatus();
	void StartRushKnockback();
	void UpdateRushKnockback();
	void EndRushKnockback();
	void RecoverRotation();

	/// <summary>
	/// ゲームオーバー演出
	/// </summary>
	void UpdateGameOverEffect();

	/// <summary>
	/// ゲームクリア演出
	/// </summary>
	void UpdateGameClearEffect();

private:
	// --- 参照 ---
	Player* player_;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 各ステータス ---
	bool isAlive_ = true;

	// ゲーム状態
	GameState gameState_ = GameState::kPlaying;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// 行動状態
	Behavior behavior_ = Behavior::kRoot;

	// kRoot関連変数
	Vector3 velocity_ = { 0.0f,0.0f,0.0f };
	float shortDistance_ = 1.5f;
	float approachSpeed_ = 0.05f;
	float maxSpeed_ = 0.08f;

	// 突進攻撃関連変数
	uint32_t chargeTimer_ = 0;
	uint32_t chargeDuration_ = 0;
	uint32_t cooldownTimer_ = 0;
	uint32_t chargeCount_ = 0;
	uint32_t maxChargeCount_ = 1;
	Vector3 chargeDirection_ = { 0.0f, 0.0f, 1.0f };
	Vector3 chargeStartPos_ = { 0.0f, 0.0f, 0.0f };

	static constexpr uint32_t kChargePreparationTime_ = 180;
	static constexpr uint32_t kChargeDuration_ = 60;
	static constexpr uint32_t kCooldownTime_ = 300;
	static constexpr float kChargeSpeed_ = 0.3f;
	static constexpr float kChargeRange_ = 15.0f;

	// 被弾時のノックバック
	bool isBeingRushed_ = false;
	uint32_t rushKnockbackTimer_ = 0;
	Vector3 knockbackDirection_ = { 0.0f, 0.0f, 1.0f };
	float knockbackSpeed_ = 0.02f;
	Vector3 originalRotation_;

	static constexpr float initialKnockbackSpeed_ = 0.02f;
	static constexpr float minKnockbackSpeed_ = 0.005f;
	static constexpr float knockbackDecay_ = 0.98f;
	static constexpr float maxTiltAngle_ = 0.3f;

	// --- 各エフェクト・演出 ---
	bool isStart_ = false;

	float fallTimer_ = 0.0f;
	const float kFallDuration_ = 60.0f;
	Vector3 fallStartPos_ = { 0.0f, 10.0f, 15.0f };
	Vector3 fallEndPos_ = { 0.0f, 2.0f, 15.0f };
	bool isFallComplete_ = false;

	// ゲームクリア演出関連変数
	float clearEffectTimer_ = 0.0f;

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static uint32_t nextSerialNumber_;
};