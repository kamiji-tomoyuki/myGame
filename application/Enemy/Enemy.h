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

public:

	Enemy();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Player* player, const ViewProjection &vp);
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
	Behavior GetBehavior() const { return behavior_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetIsStart(bool isStart) { isStart_ = isStart; }
	void SetIsGame(const bool isGame) { isGame_ = isGame; }

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

private:
	// --- 参照 ---
	Player* player_;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 各ステータス ---
	bool isGame_ = true;
	bool isAlive_ = true;

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
	uint32_t chargeTimer_ = 0;              // 突進準備タイマー
	uint32_t chargeDuration_ = 0;           // 突進継続時間
	uint32_t cooldownTimer_ = 0;            // クールダウンタイマー
	uint32_t chargeCount_ = 0;              // 現在の突進回数
	uint32_t maxChargeCount_ = 1;           // 最大突進回数
	Vector3 chargeDirection_ = { 0.0f, 0.0f, 1.0f }; // 突進方向
	Vector3 chargeStartPos_ = { 0.0f, 0.0f, 0.0f };  // 突進開始位置

	static constexpr uint32_t kChargePreparationTime_ = 180;  // 突進準備時間（3秒 @ 60FPS）
	static constexpr uint32_t kChargeDuration_ = 60;          // 突進継続時間（1秒 @ 60FPS）
	static constexpr uint32_t kCooldownTime_ = 300;           // クールダウン時間（5秒 @ 60FPS）
	static constexpr float kChargeSpeed_ = 0.3f;              // 突進速度
	static constexpr float kChargeRange_ = 15.0f;             // 突進可能距離

	// 被弾時のノックバック
	bool isBeingRushed_ = false;              // ラッシュ攻撃を受けているかどうか
	uint32_t rushKnockbackTimer_ = 0;         // ラッシュノックバックタイマー
	Vector3 knockbackDirection_ = { 0.0f, 0.0f, 1.0f }; // ノックバック方向
	float knockbackSpeed_ = 0.02f;            // 現在のノックバック速度
	Vector3 originalRotation_;                // 元の回転角度

	static constexpr float initialKnockbackSpeed_ = 0.02f;  // 初期ノックバック速度
	static constexpr float minKnockbackSpeed_ = 0.005f;     // 最小ノックバック速度
	static constexpr float knockbackDecay_ = 0.98f;         // ノックバック速度減衰率
	static constexpr float maxTiltAngle_ = 0.3f;            // 最大傾き角度（ラジアン）

	// --- 各エフェクト・演出 ---

	bool isStart_ = false;

	float fallTimer_ = 0.0f;
	const float kFallDuration_ = 60.0f;  // 1秒間の落下（60FPS想定）
	Vector3 fallStartPos_ = { 0.0f, 10.0f, 15.0f };  // 落下開始位置（高い位置）
	Vector3 fallEndPos_ = { 0.0f, 2.0f, 15.0f };     // 落下終了位置
	bool isFallComplete_ = false;  // 落下完了フラグ

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static uint32_t nextSerialNumber_;
};