#pragma once
#include "Vector3.h"
#include <cstdint>

class ParticleEmitter;

/// <summary>
/// プレイヤー被弾リアクション管理
/// </summary>
class PlayerHitReaction
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 被弾開始（ダメージを受けたフレームに呼ぶ）
	/// </summary>
	/// <param name="currentPosition">被弾時のワールド座標</param>
	/// <param name="hitPosition">エフェクト表示位置</param>
	void Start(const Vector3& currentPosition, const Vector3& hitPosition);

	/// <summary>
	/// 毎フレーム更新
	/// </summary>
	/// <returns>シェイク中の位置オフセット（座標に加算して使う）</returns>
	Vector3 Update();

	/// <summary>
	/// 接触ダメージクールダウンの更新（毎フレーム呼ぶ）
	/// </summary>
	void UpdateContactCooldown();

public:

	bool     IsHitReacting()           const { return isHitReacting_; }
	bool     IsContactCooldownActive() const { return contactDamageCooldown_ > 0; }
	Vector3  GetOriginalPosition()     const { return originalPosition_; }
	Vector3  GetShakeOffset()          const { return hitShakeOffset_; }

	void SetDamageEmitter(ParticleEmitter* emitter) { damageEffect_ = emitter; }

private:

	bool    isHitReacting_ = false;
	int     hitReactionTimer_ = 0;
	Vector3 hitShakeOffset_ = {};
	Vector3 originalPosition_ = {};
	int     contactDamageCooldown_ = 0;

	// エフェクト（所有はしない。Playerが持つ emitter をそのまま使う）
	ParticleEmitter* damageEffect_ = nullptr;

	static constexpr int   kHitReactionDuration_ = 15;
	static constexpr float kHitShakeIntensity_ = 0.15f;
	static constexpr int   kContactDamageCooldownDuration_ = 90;
};