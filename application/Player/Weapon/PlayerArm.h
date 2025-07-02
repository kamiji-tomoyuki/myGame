#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

class Player;

class PlayerArm : public BaseObject
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

	PlayerArm();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init(std::string filePath);

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
#pragma region getter setter

	/// 各ステータス取得関数
	/// <returns></returns>
	int GetID() { return serialNumber_; }
	Player* GetPlayer() { return player_; }
	bool GetIsAttack() { return isAttack_; }
	Vector3 GetCenterPosition() const override { return GetWorldPosition(); }
	Vector3 GetCenterRotation() const override { return GetWorldRotation(); }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetID(int id) { serialNumber_ = id; }
	void SetPlayer(Player* player);
	void SetComboTimer(uint32_t comboT) { comboTimer_ = comboT; }

	void SetTranslation(Vector3 pos) { transform_.translation_ = pos; }
	void SetTranslationY(float pos) { transform_.translation_.y = pos; }
	void SetTranslationX(float pos) { transform_.translation_.x = pos; }
	void SetTranslationZ(float pos) { transform_.translation_.z = pos; }

	void SetRotation(Vector3 rotate) { transform_.rotation_ = rotate; }
	void SetRotationX(float rotate) { transform_.rotation_.x = rotate; }
	void SetRotationY(float rotate) { transform_.rotation_.y = rotate; }
	void SetRotationZ(float rotate) { transform_.rotation_.z = rotate; }

	void SetScale(Vector3 scale) { transform_.scale_ = scale; }

	void SetColliderSize(float size) { Collider::SetRadius(size); }

#pragma endregion
private:

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;

	// --- 各ステータス ---
	bool isAttack_ = false;

	// コンボ
	uint32_t comboTimer_ = 0;

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// --- 各ポインタ ---
	Player* player_ = nullptr;
};

