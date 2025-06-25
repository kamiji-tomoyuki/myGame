#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

#include "Player.h"

class Weapon : public BaseObject
{
public:

	Weapon();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init()override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;
	void UpdateComboTime();

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

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetPlayer(Player* player);

private:

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	// --- 各ステータス ---
	bool isAttack_ = false;

	// --- 各ポインタ ---
	Player* player_ = nullptr;
};

