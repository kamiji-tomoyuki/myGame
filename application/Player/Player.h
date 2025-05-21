#pragma once
#include "BaseObject.h"

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
};