#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

#include "Player.h"

class Weapon : public BaseObject
{
public:

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kNormal,		// 通常
		kSkill,			// スキル
	};

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
	void SetComboTimer(uint32_t comboT) { comboTimer_ = comboT; }

private:

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	// --- 各ステータス ---
	bool isAttack_ = false;

	// コンボ
	uint32_t comboTimer_ = 0;

	// --- 各ポインタ ---
	Player* player_ = nullptr;
};

