#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include <Sprite.h>

class Player;
class EnemyAttackManager;

/// <summary>
/// 敵クラス
/// </summary>
class Enemy : public BaseObject
{
public:

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
	void DrawParticle(const ViewProjection& viewProjection);
	void DrawSprite(const ViewProjection& viewProjection);

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

	/// <summary>
	/// プレイヤーとの衝突処理
	/// </summary>
	void HandleCollisionWithPlayer(Player* player);

	void TakeDamage(uint32_t damage);

public:

	/// 各ステータス取得関数
	Vector3 GetCenterPosition() const override { return transform_.translation_; }
	Vector3 GetCenterRotation() const override { return transform_.rotation_; }
	uint32_t GetSerialNumber() const { return serialNumber_; }
	static uint32_t GetNextSerialNumber() { return nextSerialNumber_; }
	float GetShortDistance() { return shortDistance_; }
	uint32_t GetHP() { return HP_; }
	bool GetIsAlive() { return isAlive_; }
	GameState GetGameState() const { return gameState_; }
	Vector3 GetVelocity() const { return velocity_; }
	bool IsAttacking() const;
	EnemyAttackManager* GetAttackManager() const { return attackManager_.get(); }

	/// 各ステータス設定関数
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetIsStart(bool isStart) { isStart_ = isStart; }
	void SetGameState(GameState state) { gameState_ = state; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

private:

	/// <summary>
	/// 移動
	/// </summary>
	void Approach();

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
	Player* player_ = nullptr;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 攻撃管理 ---
	std::unique_ptr<EnemyAttackManager> attackManager_;

	// --- 各ステータス ---
	bool isAlive_ = true;

	// ゲーム状態
	GameState gameState_ = GameState::kPlaying;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// kRoot関連変数
	Vector3 velocity_ = { 0.0f,0.0f,0.0f };
	float shortDistance_ = 1.5f;
	float approachSpeed_ = 0.05f;
	float maxSpeed_ = 0.08f;

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
	Vector3 fallEndPos_ = { 0.0f, 0.0f, 15.0f };
	bool isFallComplete_ = false;

	// ゲームクリア演出関連変数
	float clearEffectTimer_ = 0.0f;

	// --- HPバー（背景・本体）---
	std::unique_ptr<Sprite> hpBarBg_;   // ← 追加: HPバー背景
	std::unique_ptr<Sprite> hpBar_;
	static constexpr float kHpBarFullWidth_ = 350.0f;
	static constexpr float kHpBarHeight_ = 40.0f;
	static constexpr float kHpBarBgPadding_ = 4.0f;   // ← 追加: 背景の余白

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static uint32_t nextSerialNumber_;
};