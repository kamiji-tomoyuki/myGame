#pragma once
#include "Vector3.h"
#include <cstdint>

class StageManager;
class ParticleEmitter;
class FollowCamera;

/// <summary>
/// プレイヤー移動・軌跡エフェクト管理
/// </summary>
class PlayerMove
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="stageManager">ステージ境界チェック用（非所有）</param>
	void Init(StageManager* stageManager);

	/// <summary>
	/// 移動更新
	/// </summary>
	/// <param name="currentWorldPos">現在のワールド座標</param>
	/// <param name="currentRotationY">現在のY軸回転（rad）</param>
	/// <param name="outNewPos">移動後の座標（出力）</param>
	/// <param name="outNewRotY">移動後のY軸回転（出力）</param>
	/// <returns>回避入力があった場合の移動方向（ゼロベクトルなら回避なし）</returns>
	Vector3 Update(
		const Vector3& currentWorldPos,
		float          currentRotationY,
		Vector3& outNewPos,
		float& outNewRotY);

	/// <summary>
	/// 軌跡エフェクト更新（Moveの内部から呼ぶ）
	/// </summary>
	void UpdateTrailEffect(const Vector3& currentWorldPos);

public:

	Vector3 GetVelocity()       const { return velocity_; }
	bool    IsDodgeRequested()  const { return dodgeRequested_; }
	Vector3 GetDodgeDirection() const { return dodgeDirection_; }

	void SetVelocity(const Vector3& v) { velocity_ = v; }
	void SetTrailEmitter(ParticleEmitter* emitter) { trailEffect_ = emitter; }
	void SetLastTrailPosition(const Vector3& pos) { lastTrailPosition_ = pos; }

private:

	Vector3 velocity_ = {};
	bool    dodgeRequested_ = false;
	Vector3 dodgeDirection_ = {};

	// 軌跡パーティクル
	ParticleEmitter* trailEffect_ = nullptr;
	Vector3          lastTrailPosition_ = {};
	float            trailEmitDistance_ = 0.5f;
	bool             isTrailActive_ = false;

	StageManager* stageManager_ = nullptr;

	float kAcceleration_ = 0.1f;
	float kMaxSpeed_ = 0.1f;
	float kRotateAcceleration_ = 0.1f;

	static constexpr float kFootOffsetY_ = -0.8f;
};