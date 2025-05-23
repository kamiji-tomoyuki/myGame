#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

class FollowCamera;

class Player :public BaseObject
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

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	
	/// 各ステータス設定関数
	/// <returns></returns>
	void SetFollowCamera(FollowCamera* followCamera) { followCamera_ = followCamera; }

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
	Vector3 velocity_{};
	float kAcceleration = 0.1f;
	const float kMaxSpeed = 0.1f;
	float kRotateAcceleration = 0.1f;

	// --- 各ポインタ ---
	FollowCamera* followCamera_ = nullptr;
};