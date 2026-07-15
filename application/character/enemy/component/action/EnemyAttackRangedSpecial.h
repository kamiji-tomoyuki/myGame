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

/// <summary>
/// 敵の特殊遠距離攻撃クラス (背後からトゲをホーミング射出)
/// </summary>
class EnemyAttackRangedSpecial
{
public:
	enum class Phase {
		kNone,
		kPreparation,	// 予備動作 (下を向いてシェイク、トゲ出現)
		kJump,			// 軽くジャンプ
		kLaunch,		// 着地と同時に発射
		kRecovery,		// 硬直
	};

	/// <summary>
	/// 飛ばすトゲの構造体
	/// </summary>
	struct SpikeProjectile {
		WorldTransform transform;
		std::unique_ptr<Object3d> model;
		Vector3 velocity;
		bool isActive = false;
		bool isLaunched = false;
		uint32_t launchDelay = 0;		// 発射までの遅延
		uint32_t spawnDelay = 0;		// 出現までの遅延
		uint32_t spawnTimer = 0;		// 出現演出用タイマー
		uint32_t lifeTimer = 0;
		bool hasHit = false;
		float currentSpeed = 0.0f;
		Vector3 startOffset;			// 配置時のオフセット
	};

public:
	EnemyAttackRangedSpecial();
	~EnemyAttackRangedSpecial();

	void Initialize();
	void Start(Enemy* enemy, Player* player);
	void Update(Enemy* enemy, Player* player);
	void UpdateViewProjection(const ViewProjection& vp);
	void Interrupt();
	void Draw(const ViewProjection& viewProjection);

	bool IsComplete() const { return isComplete_ && projectiles_.empty(); }

private:
	void UpdatePreparation(Enemy* enemy, Player* player);
	void UpdateJump(Enemy* enemy);
	void UpdateLaunch(Enemy* enemy, Player* player);
	void UpdateRecovery(Enemy* enemy);
	void UpdateProjectiles(Player* player);
	void ApplyVariables();

	// トゲを背後に配置する
	void ArrangeSpikes(Enemy* enemy, Player* player);

private:
	Phase phase_ = Phase::kNone;
	bool isComplete_ = false;

	uint32_t timer_ = 0;
	uint32_t launchTimer_ = 0;			// 発射管理用タイマー
	float jumpVelocityY_ = 0.0f;
	Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };
	Vector3 basePosition_ = { 0.0f, 0.0f, 0.0f };

	std::vector<SpikeProjectile> projectiles_;

	const ViewProjection* vp_ = nullptr;
	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;

	// -------------------------------------------------------
	// 調整可能パラメータ（GlobalVariables から ApplyVariables で上書きされる）
	// -------------------------------------------------------
	uint32_t prepTime_ = 45;		// 詠唱時間
	uint32_t launchDuration_ = 180; // 発射にかける時間 (さらに長く、バラバラに)
	uint32_t recoveryTime_ = 40;	// 発射後硬直

	float spikeSpeed_ = 0.35f;		// トゲの弾速
	float homingStrength_ = 0.015f; // ホーミングの強さ (吸い付きを弱く)
	uint32_t spikeLifeTime_ = 180; // 消滅までの時間
	uint32_t spikeCount_ = 10;		// トゲの数 (さらに増やす)
	int32_t damage_ = 8;			// ダメージ (さらに複数ヒット前提で調整)

	// -------------------------------------------------------
	// 固定パラメータ（GlobalVariables には登録していない）
	// -------------------------------------------------------
	static constexpr uint32_t kJumpTime = 15;		// ジャンプ上昇時間
	static constexpr uint32_t kSpawnTime = 20;		// 出現演出にかかる時間
	static constexpr float kJumpPower = 0.4f;		// ジャンプ力
	static constexpr float kGravity = 0.04f;		// ジャンプ中の重力
	static constexpr float kLookDownAngle = 0.3f;	// 予備動作時の下向き角度
	static constexpr float kShakeAmount = 0.18f;	// シェイク幅 (大幅に強化)
};
