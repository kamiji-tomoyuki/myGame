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
		kAttack,	// 攻撃
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
	void Update(Player* player);

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

	/// 各ステータス設定関数
	/// <returns></returns>
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }

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

private:
	// --- 参照 ---
	Player* player_;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 各ステータス ---
	bool isAlive_ = true;

	// HP
	uint32_t kMaxHP_ = 1000;
	uint32_t HP_ = kMaxHP_;

	// kRoot関連変数
	Vector3 velocity_ = { 0.0f,0.0f,0.0f };
	float shortDistance_ = 1.5f;
	float approachSpeed_ = 0.05f;
	float maxSpeed_ = 0.08f;

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

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static uint32_t nextSerialNumber_;
};