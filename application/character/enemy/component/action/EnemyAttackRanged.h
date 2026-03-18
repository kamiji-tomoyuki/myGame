#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include "GlobalVariables.h"
#include <vector>
#include <memory>

class Enemy;
class Player;
class ViewProjection;

class EnemyAttackRanged
{
public:
	enum class Phase {
		kNone,
		kPreparation,	// 予備動作(詠唱)
		kAttacking,		// 攻撃中
		kRecovery,		// 硬直(回復)
	};

	/// <summary>
	/// 遠距離攻撃インスタンス
	/// </summary>
	struct RangedAttackInstance {
		Vector3 position;					// 出現位置
		uint32_t warningTimer = 0;			// 警告表示タイマー
		uint32_t spikeTimer = 0;			// トゲ出現タイマー
		bool isWarningActive = false;
		bool isSpikeActive = false;
		float spikeHeight = 0.0f;			// トゲの高さ

		std::unique_ptr<Object3d> warningCircle;	// 赤い円形ジオグリフ
		std::unique_ptr<Object3d> spike;			// トゲモデル
	};

public:
	EnemyAttackRanged();
	~EnemyAttackRanged();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 攻撃開始
	/// </summary>
	void Start(Enemy* enemy, Player* player);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(Enemy* enemy, Player* player);

	/// <summary>
	/// ビュープロジェクション更新
	/// </summary>
	void UpdateViewProjection(const ViewProjection& vp);

	/// <summary>
	/// 攻撃中断
	/// </summary>
	void Interrupt();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection);

public:
	// Getter
	bool IsComplete() const { return isComplete_; }
	Phase GetPhase() const { return phase_; }
	const std::vector<RangedAttackInstance>& GetAttackInstances() const { return attackInstances_; }

private:
	void UpdatePreparation(Enemy* enemy);
	void UpdateAttacking(Enemy* enemy, Player* player);
	void UpdateRecovery(Enemy* enemy);
	void UpdateAttackInstances(Player* player);
	void CheckCollision(Player* player);
	void ApplyVariables();

private:
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	uint32_t preparationTimer_ = 0;
	uint32_t attackTimer_ = 0;
	uint32_t recoveryTimer_ = 0;
	uint32_t attackPhase_ = 0;

	std::vector<RangedAttackInstance> attackInstances_;

	Vector3 originalRotation_ = { 0.0f, 0.0f, 0.0f };

	const ViewProjection* vp_ = nullptr;

	// -------------------------------------------------------
	// GlobalVariables で調整可能な変数（constexpr から昇格）
	// -------------------------------------------------------
	uint32_t kPreparationTime_ = 45;   // 予備動作時間
	uint32_t kRecoveryTime_ = 40;   // 回復時間
	uint32_t kAttackInterval_ = 40;   // ジオグリフ出現間隔
	uint32_t kAttackCount_ = 3;    // 攻撃回数
	uint32_t kWarningDuration_ = 30;   // 警告表示時間
	uint32_t kSpikeRiseDuration_ = 15;   // トゲの上昇時間
	uint32_t kSpikeHoldDuration_ = 30;   // トゲの持続時間
	uint32_t kSpikeFallDuration_ = 20;   // トゲの下降時間
	float    kSpikeMaxHeight_ = 3.0f; // トゲの最大高さ
	float    kWarningCircleRadius_ = 2.0f; // 警告円の半径
	float    kPreparationTiltAngle_ = 0.4f; // 予備動作時の後傾角度
	int32_t  kRangedDamage_ = 100;  // 遠距離攻撃ダメージ

	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
};