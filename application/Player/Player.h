#pragma once
#include "BaseObject.h"

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

	// --- 各ステータス ---
	float kAcceleration;

	// --- 各ポインタ ---
	FollowCamera* followCamera_ = nullptr;
};