#pragma once
#include "Vector3.h"
#include "Object3d.h"
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
	/// <summary>
	/// 予備動作更新
	/// </summary>
	void UpdatePreparation(Enemy* enemy);

	/// <summary>
	/// 攻撃更新
	/// </summary>
	void UpdateAttacking(Enemy* enemy, Player* player);

	/// <summary>
	/// 回復動作更新
	/// </summary>
	void UpdateRecovery(Enemy* enemy);

	/// <summary>
	/// 攻撃インスタンス更新
	/// </summary>
	void UpdateAttackInstances(Player* player);

	/// <summary>
	/// 当たり判定チェック
	/// </summary>
	void CheckCollision(Player* player);

private:
	// フェーズ
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	// タイマー
	uint32_t preparationTimer_ = 0;
	uint32_t attackTimer_ = 0;
	uint32_t recoveryTimer_ = 0;
	uint32_t attackPhase_ = 0;

	// 攻撃インスタンス
	std::vector<RangedAttackInstance> attackInstances_;

	// 元の回転
	Vector3 originalRotation_ = { 0.0f, 0.0f, 0.0f };

	// ビュープロジェクション参照
	const ViewProjection* vp_ = nullptr;

	// 定数
	static constexpr uint32_t kPreparationTime_ = 45;		// 予備動作時間
	static constexpr uint32_t kRecoveryTime_ = 40;			// 回復時間
	static constexpr uint32_t kAttackInterval_ = 40;		// ジオグリフ出現間隔
	static constexpr uint32_t kAttackCount_ = 3;			// 攻撃回数
	static constexpr uint32_t kWarningDuration_ = 30;		// 警告表示時間
	static constexpr uint32_t kSpikeRiseDuration_ = 15;		// トゲの上昇時間
	static constexpr uint32_t kSpikeHoldDuration_ = 30;		// トゲの持続時間
	static constexpr uint32_t kSpikeFallDuration_ = 20;		// トゲの下降時間
	static constexpr float kSpikeMaxHeight_ = 3.0f;			// トゲの最大高さ
	static constexpr float kWarningCircleRadius_ = 2.0f;	// 警告円の半径
	static constexpr float kPreparationTiltAngle_ = 0.4f;	// 予備動作時の後傾角度
};