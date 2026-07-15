#pragma once
#include "Vector3.h"
#include "PlayerArm.h"
#include <cstdint>
#include <array>
#include <memory>

using namespace Engine;
class FollowCamera;
class StageManager;

/// <summary>
/// ラッシュ中のプレイヤー姿勢制御・フィニッシャー前進管理
/// </summary>
class PlayerRushPosture
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init(FollowCamera* followCamera, StageManager* stageManager);

	/// <summary>
	/// ラッシュ姿勢更新
	/// </summary>
	/// <param name="arms">プレイヤーの腕配列</param>
	/// <param name="currentRotation">現在のプレイヤー回転</param>
	/// <param name="outRotation">適用すべき新しい回転（出力）</param>
	void UpdateBodyPosture(
		const std::array<std::unique_ptr<PlayerArm>, 2>& arms,
		const Vector3& currentRotation,
		Vector3& outRotation);

private:

	FollowCamera* followCamera_ = nullptr;
	StageManager* stageManager_ = nullptr;

	// 姿勢制御
	Vector3 rushBaseRotation_ = {};
	float   rushBodyPitchTarget_ = 0.0f;
	float   rushBodyPitchCurrent_ = 0.0f;
	float   rushBodyTwistTarget_ = 0.0f;
	float   rushBodyTwistCurrent_ = 0.0f;

	static constexpr float kRushLeanPitch_ = 0.18f;
	static constexpr float kWindUpTwist_ = 0.22f;
	static constexpr float kFinisherTwist_ = -0.30f;
	static constexpr float kChargeLeanBackPitch_ = 0.50f;    // 溜めの後傾量（X負方向へ倒す）
	static constexpr float kAttackLeanForwardPitch_ = 0.4f; // 連打攻撃の前傾量（X正方向へ倒す）
};