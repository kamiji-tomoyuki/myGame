#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include "GlobalVariables.h"
#include <vector>
#include <memory>

using namespace Engine;
class Enemy;
class Player;
namespace Engine { class ViewProjection; }

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
		bool hasHitPlayer = false;			// このトゲで既にダメージを与えたか

		std::unique_ptr<Object3d> warningOutline;	// 警告円の輪郭
		std::unique_ptr<Object3d> warningFill;		// 警告円の塗りつぶし
		std::unique_ptr<Object3d> spike;			// トゲモデル

		WorldTransform warningTransform;	// 警告円(輪郭)用トランスフォーム
		WorldTransform warningFillTransform;// 警告円(塗りつぶし)用トランスフォーム
		WorldTransform spikeTransform;		// トゲ用トランスフォーム
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
	Vector3 shakeBasePosition_ = { 0.0f, 0.0f, 0.0f };

	const ViewProjection* vp_ = nullptr;

	// -------------------------------------------------------
	// GlobalVariables で調整可能な変数（constexpr から昇格）
	// -------------------------------------------------------
	uint32_t preparationTime_ = 45;   // 予備動作時間
	uint32_t recoveryTime_ = 40;   // 回復時間
	uint32_t attackInterval_ = 40;   // ジオグリフ出現間隔
	uint32_t attackCount_ = 3;    // 攻撃回数
	uint32_t warningDuration_ = 30;   // 警告表示時間
	uint32_t spikeRiseDuration_ = 15;   // トゲの上昇時間
	uint32_t spikeHoldDuration_ = 30;   // トゲの持続時間
	uint32_t spikeFallDuration_ = 20;   // トゲの下降時間
	float    spikeMaxHeight_ = 3.0f; // トゲの最大高さ
	float    warningCircleRadius_ = 2.0f; // 警告円の半径
	float    preparationTiltAngle_ = 0.4f; // 予備動作時の後傾角度
	int32_t  rangedDamage_ = 100;  // 遠距離攻撃ダメージ

	const float kShakeAmount_ = 0.04f;  // 攻撃中シェイク幅
	const float kShakeSpeed_ = 0.7f;   // 攻撃中シェイク速度
	const float kShakeSpeedZScale_ = 1.3f;   // Z方向シェイク速度の係数
	const float kSpikeHitHeightRatio_ = 0.5f;   // ヒット判定に必要なトゲ高さ割合
	const float kSpikeGroundOffsetY_ = 0.01f;   // トゲ出現時の地面からのYオフセット
	const float kWarningCircleRotationX_ = 1.57f;  // 警告円のX軸回転角（90度）
	const float kSpikeCenterOffsetScale_ = 1.0f;   // トゲ描画位置のY中心オフセット係数

	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
};