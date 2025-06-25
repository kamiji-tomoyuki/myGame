#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

#include <Stage/StageManager.h>
#include <ParticleEmitter.h>

class FollowCamera;

class Player : public BaseObject
{
public:

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kRoot,		// 通常
		kAttack,	// 攻撃
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

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }
	void SetViewProjection(const ViewProjection* viewProjection) { vp_ = viewProjection; }
	static void SetSerialNumber(int num) { nextSerialNumber_ = num; }
	void SetTranslation(const Vector3& translation) { transform_.translation_ = translation; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

private:

	/// <summary>
	/// 移動
	/// </summary>
	void Move();

private:
	
	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 各ステータス ---
	bool isAlive_ = true;

	// Move関連変数
	bool isMove_ = false;
	Vector3 velocity_{};
	float kAcceleration_ = 0.1f;
	const float kMaxSpeed_ = 0.1f;
	float kRotateAcceleration_ = 0.1f;

	// Attack関連変数
	bool isAttack_ = false;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> hitEffect_;

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// --- 各ポインタ ---
	FollowCamera* followCamera_ = nullptr;

	// --- ステージマネージャー ---
	StageManager* stageManager_ = nullptr;
};