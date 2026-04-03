#include "PlayerHitReaction.h"
#include <ParticleEmitter.h>
#include <random>

void PlayerHitReaction::Init()
{
	isHitReacting_ = false;
	hitReactionTimer_ = 0;
	hitShakeOffset_ = {};
	originalPosition_ = {};
	contactDamageCooldown_ = 0;
	rangedDamageCooldown_ = 0;
}

// =============================================================
//  被弾開始
// =============================================================
void PlayerHitReaction::Start(const Vector3& currentPosition, const Vector3& hitPosition)
{
	isHitReacting_ = true;
	hitReactionTimer_ = 0;
	originalPosition_ = currentPosition;

	if (damageEffect_) {
		damageEffect_->SetPosition(hitPosition);
		damageEffect_->SetActive(false);
	}

	contactDamageCooldown_ = kContactDamageCooldownDuration_;
}

// =============================================================
//  更新
//  戻り値: シェイクオフセット（リアクション終了後は {0,0,0}）
// =============================================================
Vector3 PlayerHitReaction::Update()
{
	if (!isHitReacting_) { return {}; }

	hitReactionTimer_++;

	static std::random_device                    rd;
	static std::mt19937                          gen(rd());
	static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	float intensity = kHitShakeIntensity_ *
		(1.0f - static_cast<float>(hitReactionTimer_) / static_cast<float>(kHitReactionDuration_));

	hitShakeOffset_ = {
		dist(gen) * intensity,
		dist(gen) * intensity,
		dist(gen) * intensity
	};

	if (hitReactionTimer_ >= kHitReactionDuration_) {
		isHitReacting_ = false;
		hitReactionTimer_ = 0;
		hitShakeOffset_ = {};
	}

	return hitShakeOffset_;	// 終了フレームは {} が返る
}

// =============================================================
//  接触ダメージクールダウン更新
// =============================================================
void PlayerHitReaction::UpdateContactCooldown()
{
	if (contactDamageCooldown_ > 0) {
		contactDamageCooldown_--;
	}
}

// =============================================================
//  遠距離攻撃被弾クールダウン更新
// =============================================================
void PlayerHitReaction::UpdateRangedCooldown()
{
	if (rangedDamageCooldown_ > 0) {
		rangedDamageCooldown_--;
	}
}