#pragma once
#include "BaseObject.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include <CollisionTypeIdDef.h>

class Player;

/// <summary>
/// プレイヤー(腕)クラス
/// </summary>
class PlayerArm : public BaseObject
{
public:

	/// <summary>
	/// 状態
	/// </summary>
	enum class Behavior {
		kNormal,		// 通常
		kAttack,		// 攻撃中
		kSkill,			// スキル
		kRush,			// ラッシュ攻撃中
	};

	/// <summary>
	/// 攻撃の種類
	/// </summary>
	enum class AttackType {
		kNone,			// 攻撃なし
		kRightPunch,	// 右パンチ
		kLeftPunch,		// 左パンチ
		kRush,			// ラッシュ攻撃
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
	void UpdateAttack();
	void UpdateRush();

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

	/// <summary>
	/// 攻撃処理
	/// </summary>
	void StartAttack(AttackType attackType);
	void StartRush();
	void ProcessAttack();
	bool CanCombo() const;
	bool CanStartRush() const;

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
	bool GetIsRush() { return isRush_; }
	AttackType GetCurrentAttackType() { return currentAttackType_; }
	AttackType GetLastAttackType() { return lastAttackType_; }
	Behavior GetBehavior() { return behavior_; }
	Vector3 GetCenterPosition() const override { return GetWorldPosition(); }
	Vector3 GetCenterRotation() const override { return GetWorldRotation(); }
	Vector3 GetAttackDirection() const { return attackDirection_; }
	uint32_t GetComboCount() const { return comboCount_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetID(int id) { serialNumber_ = id; }
	void SetColliderID(CollisionTypeIdDef id) { Collider::SetTypeID(static_cast<uint32_t>(id)); }
	void SetPlayer(Player* player);
	void SetComboTimer(uint32_t comboT) { comboTimer_ = comboT; }
	void SetComboCount(uint32_t count) { comboCount_ = count; }

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
	bool isRush_ = false;
	Behavior behavior_ = Behavior::kNormal;
	AttackType currentAttackType_ = AttackType::kNone;
	AttackType lastAttackType_ = AttackType::kNone;

	// 攻撃関連
	Vector3 attackDirection_ = { 0.0f, 0.0f, 1.0f };  // 攻撃方向（正面）
	Vector3 originalPosition_;		// 元の位置
	Vector3 targetPosition_;		// 攻撃時の目標位置
	float attackProgress_ = 0.0f;	// 攻撃の進行度（0.0f〜1.0f）
	uint32_t attackDamage_;			// ダメージ
	bool hasHitThisAttack_ = false; // この攻撃で既にダメージを与えたか

	// ラッシュ関連
	uint32_t rushTimer_ = 0;        // ラッシュ攻撃継続時間
	uint32_t rushAttackTimer_ = 0;  // 個別のラッシュ攻撃タイマー
	uint32_t rushCount_ = 0;        // ラッシュ攻撃回数カウント
	bool rushAttackActive_ = false; // 個別のラッシュ攻撃が実行中かどうか
	uint32_t rushAttackDamage_;		// ダメージ

	// ラッシュ連続ヒット防止用
	int lastRushHitFrame_ = -999;   // 前回のラッシュヒットフレーム

	// タイマー関連
	uint32_t attackTimer_ = 0;      // 攻撃アニメーション用タイマー
	uint32_t comboTimer_ = 0;       // コンボ受付時間
	uint32_t comboCount_ = 0;       // コンボ数

	// 定数
	static constexpr uint32_t kAttackDuration = 20;		// 攻撃アニメーション時間（フレーム）
	static constexpr uint32_t kComboWindow = 30;		// コンボ受付時間（フレーム）
	static constexpr uint32_t kRushDuration = 120;		// ラッシュ攻撃継続時間（フレーム）
	static constexpr uint32_t kRushInterval = 8;		// ラッシュ攻撃間隔（フレーム）
	static constexpr uint32_t kRushAttackDuration = 12; // 個別ラッシュ攻撃時間（フレーム）
	static constexpr float kAttackDistance = 2.0f;		// 攻撃時の前進距離
	static constexpr float kRushDistance = 1.5f;		// ラッシュ攻撃時の前進距離
	static constexpr float kRightPunchOffset = -0.5f;	// 右パンチの横オフセット
	static constexpr float kLeftPunchOffset = 0.5f;		// 左パンチの横オフセット

	// シリアルナンバー
	uint32_t serialNumber_ = 0;
	static inline int nextSerialNumber_ = 0;

	// --- 各ポインタ ---
	Player* player_ = nullptr;
};