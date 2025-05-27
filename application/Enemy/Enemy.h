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

	/// 各ステータス取得関数
	/// <returns></returns>
	Vector3 GetCenterPosition() const override { return transform_.translation_; }
	Vector3 GetCenterRotation() const override { return transform_.rotation_; }

	/// 各ステータス設定関数
	/// <returns></returns>


private:

	/// <summary>
	/// 移動
	/// </summary>
	void Approach();

private:
	// --- 参照 ---
	Player* player_;

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	const ViewProjection* vp_ = nullptr;

	// --- 各ステータス ---

};

