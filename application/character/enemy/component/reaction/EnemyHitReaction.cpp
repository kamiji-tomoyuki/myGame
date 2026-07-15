#include "EnemyHitReaction.h"
#include "Enemy.h"
#include "Player.h"
#include "EnemyAttackManager.h"
#include "EnemyAttackMelee.h"
#include "StageManager.h"
#include <cmath>

using namespace Engine;
void EnemyHitReaction::CheckPlayerRushStatus(Enemy* enemy)
{
	if (enemy == nullptr) { return; }
	Player* player = enemy->GetPlayer();
	if (player == nullptr) { return; }

	bool isRushActive = player->IsRushActive();

	if (wasRushActive_ && !isRushActive) {
		if (!rushFinalHitReceived_) {
			// ラッシュが終了し、かつフィニッシュを食らっていなければノックバック終了
			EndKnockback(enemy);
		}
		rushFinalHitReceived_ = false;
	}

	wasRushActive_ = isRushActive;
}


void EnemyHitReaction::UpdateStun()
{
	rushStunTimer_--;
	if (rushStunTimer_ <= 0) {
		isRushStunned_ = false;
		rushStunTimer_ = 0;
	}
}

void EnemyHitReaction::OnRushHit(bool isFinalHit, Enemy* enemy)
{
	if (!enemy->GetIsAlive()) { return; }

	// 被弾フラッシュのみ（連続ヒットのカウントは TakeDamage 側の OnHit() が1回だけ行う。
	// ここで OnHit() を呼ぶとラッシュが二重カウントになるため呼ばない）
	isFlashing_ = true;
	flashTimer_ = kFlashDuration_;

	if (isFinalHit) {
		isRushStunned_ = true;
		rushStunTimer_ = kRushStunDuration_ * kFinalHitStunMultiplier_;
		rushFinalHitReceived_ = true; // フィニッシュを受けたフラグを立てる

		// 最後のヒットでノックバック（バウンドあり）を開始
		StartKnockback(enemy, enemy->GetPlayer(), true);
	}
	else {
		isRushStunned_ = true;
		rushStunTimer_ = kRushStunDuration_;
	}

	// 攻撃を中断させる（全ヒットで中断させる）
	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
	}
}

void EnemyHitReaction::Reset(Enemy* enemy)
{
	// スタンリセット
	isRushStunned_ = false;
	rushStunTimer_ = 0;

	// フラッシュ・連続ヒットリセット
	isFlashing_ = false;
	flashTimer_ = 0;
	consecutiveHits_ = 0;
	hitWindow_ = 0;
	repelCooldown_ = 0;
	repelPushTimer_ = 0;

	// ノックバック終了
	EndKnockback(enemy);

	wasRushActive_ = false;
	rushFinalHitReceived_ = false;
}

void EnemyHitReaction::OnHit()
{
	// 被弾時は「揺れ」ではなくモデルを一瞬赤くフラッシュ
	isFlashing_ = true;
	flashTimer_ = kFlashDuration_;

	// 連続ヒット（プレイヤーの連続攻撃）を数える
	consecutiveHits_++;
	hitWindow_ = kHitWindowFrames_;
}

void EnemyHitReaction::UpdateFlash()
{
	if (!isFlashing_) { return; }
	flashTimer_--;
	if (flashTimer_ <= 0) {
		isFlashing_ = false;
		flashTimer_ = 0;
	}
}

// =============================================================
//  連続ヒット対策：一定連続で被弾したら攻撃を中断してプレイヤーを押し返す
// =============================================================
void EnemyHitReaction::UpdateComboRepel(Enemy* enemy)
{
	if (enemy == nullptr) { return; }
	Player* player = enemy->GetPlayer();

	// 猶予タイマー：一定時間ヒットが無ければ連続数リセット
	if (hitWindow_ > 0) {
		hitWindow_--;
		if (hitWindow_ == 0) { consecutiveHits_ = 0; }
	}
	if (repelCooldown_ > 0) { repelCooldown_--; }

	// 押し出し継続（プレイヤーを敵から離す）
	if (repelPushTimer_ > 0 && player != nullptr) {
		Vector3 newPos = player->GetCenterPosition() + repelDir_ * kRepelPushSpeed_;
		newPos = StageManager::GetInstance()->ClampToStageBounds(newPos);
		player->SetWorldPosition(newPos);
		repelPushTimer_--;
	}

	// 発動判定。ラッシュ／フィニッシャー中は発動させない（それらの見せ場を潰さない）。
	//   ラッシュで貯まった連続ヒットは維持され、ラッシュ後もなお攻撃を続けると押し返される。
	if (consecutiveHits_ >= kComboRepelThreshold_ && repelCooldown_ == 0 &&
		player != nullptr && !player->IsRushActive() && !player->IsFinisherActive()) {
		DoRepel(enemy, player);
	}
}

void EnemyHitReaction::DoRepel(Enemy* enemy, Player* player)
{
	// 敵は自分の攻撃・行動を中断してでも押し返しに移る
	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
		if (auto* melee = mgr->GetMeleeAttack()) { melee->Interrupt(enemy); }
	}

	// プレイヤーを敵から離す方向へ、数フレームかけて押し出す
	Vector3 dir = player->GetCenterPosition() - enemy->GetCenterPosition();
	dir.y = 0.0f;
	repelDir_ = (dir.Length() > 0.001f) ? dir.Normalize() : Vector3(0.0f, 0.0f, -1.0f);
	repelPushTimer_ = kRepelPushFrames_;

	// プレイヤーの行動（コンボ等）を中断させる（被弾リアクション）。位置押し出しは上の timer で行う。
	player->ReceiveComboRepel(enemy->GetCenterPosition());

	// 演出：敵を赤くフラッシュ
	isFlashing_ = true;
	flashTimer_ = kFlashDuration_;

	repelCooldown_ = kRepelCooldownFrames_;
	consecutiveHits_ = 0;
	hitWindow_ = 0;
}

void EnemyHitReaction::StartKnockback(Enemy* enemy, Player* player, bool shouldBounce)
{
	isBeingRushed_ = true;
	isBouncing_ = shouldBounce;
	rushKnockbackTimer_ = 0;
	knockbackSpeed_ = initialKnockbackSpeed_;
	knockbackVerticalVelocity_ = knockbackInitialVerticalVelocity_;
	knockbackGroundY_ = enemy->GetCenterPosition().y;

	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
		// 現在のアタックを強制停止させる
		if (auto* melee = mgr->GetMeleeAttack()) { melee->Interrupt(enemy); }
	}

	// 回転のリセット（本体・モデル両方）
	Vector3 originalRot = enemy->GetOriginalRotation();
	Vector3 worldRot = enemy->GetWorldRotation();
	worldRot.x = originalRot.x;
	worldRot.z = originalRot.z;
	enemy->SetRotation(worldRot);
	enemy->SetObjRotation(originalRot);

	if (player != nullptr) {
		Vector3 direction = enemy->GetCenterPosition() - player->GetCenterPosition();
		direction.y = 0.0f;
		knockbackDirection_ = (direction.Length() > kKnockbackDirectionMinLength_)
			? direction.Normalize()
			: Vector3(0.0f, 0.0f, 1.0f);
	}
}

void EnemyHitReaction::UpdateKnockback(Enemy* enemy)
{
	rushKnockbackTimer_++;

	if (rushKnockbackTimer_ >= kMaxRushKnockbackDuration_) {
		EndKnockback(enemy);
		return;
	}

	Vector3 currentPos = enemy->GetCenterPosition();

	// 縦方向（ノックアップ・バウンド）
	knockbackVerticalVelocity_ -= knockbackGravity_;
	float newY = currentPos.y + knockbackVerticalVelocity_;

	if (newY <= knockbackGroundY_ && knockbackVerticalVelocity_ <= 0.0f) {
		newY = knockbackGroundY_;

		if (isBouncing_) {
			// 下降中に地面に触れたら反転してバウンド
			knockbackVerticalVelocity_ = -knockbackVerticalVelocity_ * kBounceFactor_;

			// バウンド速度が一定以下なら終了
			if (knockbackVerticalVelocity_ < kMinBounceVelocity_) {
				knockbackVerticalVelocity_ = 0.0f;
				isBouncing_ = false;
			}
		}
		else {
			knockbackVerticalVelocity_ = 0.0f;
		}
	}

	// 水平方向（吹っ飛び）
	Vector3 horizontalVelocity = knockbackDirection_ * knockbackSpeed_;
	knockbackSpeed_ *= knockbackDecay_;
	if (knockbackSpeed_ < 0.001f) { knockbackSpeed_ = 0.0f; }

	Vector3 newPos = currentPos + horizontalVelocity;
	newPos.y = newY;

	// ステージ境界内に制限
	newPos = StageManager::GetInstance()->ClampToStageBounds(newPos);

	enemy->SetWorldPosition(newPos);

	// 回転演出
	Vector3 worldRot = enemy->GetWorldRotation();
	Vector3 originalRot = enemy->GetOriginalRotation();
	float   tiltAmount = 0.0f;

	bool isAirborne = (newY > knockbackGroundY_ + kAirborneThreshold_);
	if (isAirborne) {
		tiltAmount = -knockbackVerticalVelocity_ * (maxTiltAngle_ / knockbackInitialVerticalVelocity_);
	}
	else {
		float decayFactor = 1.0f - static_cast<float>(rushKnockbackTimer_) / static_cast<float>(kMaxRushKnockbackDuration_);
		tiltAmount = sin(static_cast<float>(rushKnockbackTimer_) * kGroundTiltFrequency_) * maxTiltAngle_ * decayFactor * kGroundTiltDampingScale_;
	}

	// 本体を傾ける
	worldRot.x = originalRot.x + tiltAmount;
	enemy->SetRotation(worldRot);
	// モデル側の個別回転はリセットしておく
	enemy->SetObjRotation(originalRot);
}

void EnemyHitReaction::EndKnockback(Enemy* enemy)
{
	isBeingRushed_ = false;
	isBouncing_ = false;
	rushKnockbackTimer_ = 0;
	knockbackSpeed_ = 0.0f;
	knockbackVerticalVelocity_ = 0.0f;

	if (enemy) {
		Vector3 pos = enemy->GetCenterPosition();
		if (pos.y < knockbackGroundY_) {
			pos.y = knockbackGroundY_;
			enemy->SetWorldPosition(pos);
		}

		// 回転を最終リセット
		Vector3 currentRot = enemy->GetWorldRotation();
		Vector3 originalRot = enemy->GetOriginalRotation();
		enemy->SetRotation(Vector3(originalRot.x, currentRot.y, originalRot.z));
		enemy->SetObjRotation(originalRot);
	}
}