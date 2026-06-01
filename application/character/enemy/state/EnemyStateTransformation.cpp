#include "EnemyStateTransformation.h"
#include "EnemyStatePlaying.h"
#include "Enemy.h"
#include "Player.h"
#include "EnemyAttackManager.h"
#include "EnemyHitReaction.h"
#include "GlobalVariables.h"
#include <cmath>
#include <numbers>

// 手動補間用のヘルパー
static float ManualLerp(float start, float end, float t) {
	return start + (end - start) * t;
}

static Vector3 ManualVector3Lerp(const Vector3& start, const Vector3& end, float t) {
	return {
		ManualLerp(start.x, end.x, t),
		ManualLerp(start.y, end.y, t),
		ManualLerp(start.z, end.z, t)
	};
}

void EnemyStateTransformation::Enter(Enemy* enemy)
{
	enemy->SetIsInvincible(true);
	
	// 軌跡を停止
	if (auto* move = enemy->GetMove()) {
		move->StopTrail();
	}

	// 被弾リアクションを確実にリセット
	if (auto* hitReaction = enemy->GetHitReaction()) {
		hitReaction->Reset(enemy);
	}

	phase_ = Phase::kJumpUp;
	timer_ = 0;
	
	startPos_ = enemy->GetCenterPosition();
	targetPos_ = startPos_;
	targetPos_.y += kJumpHeight;

	if (auto* attackManager = enemy->GetAttackManager()) {
		attackManager->InterruptByRush(enemy);
	}
}

std::unique_ptr<IEnemyState> EnemyStateTransformation::Update(Enemy* enemy)
{
	timer_++;

	// 常にプレイヤーを向く（Y軸）
	Player* player = enemy->GetPlayer();
	if (player) {
		Vector3 enemyPos = enemy->GetCenterPosition();
		Vector3 playerPos = player->GetCenterPosition();
		Vector3 toPlayer = (playerPos - enemyPos).Normalize();
		float targetRotationY = std::atan2(toPlayer.x, toPlayer.z);
		
		// 現在の回転を取得してYだけ上書き
		Vector3 currentRot = enemy->GetWorldRotation();
		currentRot.y = targetRotationY;
		enemy->SetRotation(currentRot);
	}

	switch (phase_) {
	case Phase::kJumpUp:
		if (timer_ <= kJumpTime) {
			float t = static_cast<float>(timer_) / static_cast<float>(kJumpTime);
			// EaseIn
			float easeT = t * t; 
			enemy->SetTranslation(ManualVector3Lerp(startPos_, targetPos_, easeT));
		}
		else {
			phase_ = Phase::kLanding;
			timer_ = 0;
			// スタート演出の着地座標 (0.0, 0.0, 15.0) を参考にする
			startPos_ = { 0.0f, kJumpHeight, 15.0f };
			targetPos_ = { 0.0f, 0.0f, 15.0f }; // スタート演出の fallEndPos_
			enemy->SetTranslation(startPos_);
		}
		break;

	case Phase::kLanding:
		if (timer_ <= kLandingTime * 2) {
			float t = static_cast<float>(timer_) / static_cast<float>(kLandingTime * 2);
			// EaseOutQuad
			float easeT = t * (2.0f - t);
			enemy->SetTranslation(ManualVector3Lerp(startPos_, targetPos_, easeT));
		}
		else {
			enemy->SetTranslation(targetPos_);
			phase_ = Phase::kFloating;
			timer_ = 0;
			startPos_ = targetPos_;
			targetPos_ = startPos_;
			targetPos_.y += kFloatingHeight;
		}
		break;

	case Phase::kFloating:
		if (timer_ <= kFloatTime) {
			float t = static_cast<float>(timer_) / static_cast<float>(kFloatTime);
			float easeT = std::sin(t * (std::numbers::pi_v<float> / 2.0f));
			enemy->SetTranslation(ManualVector3Lerp(startPos_, targetPos_, easeT));
		}
		else {
			phase_ = Phase::kTilting;
			timer_ = 0;
			startPos_ = enemy->GetCenterPosition();
		}
		break;

	case Phase::kTilting:
		// 15フレームに戻す
		if (timer_ <= 15) {
			float t = static_cast<float>(timer_) / 15.0f;
			// EaseInOut
			float easeT = 0.5f * (1.0f - std::cos(t * std::numbers::pi_v<float>));
			
			Vector3 currentRot = enemy->GetWorldRotation();
			// 斜め上を向く 
			currentRot.x = ManualLerp(enemy->GetOriginalRotation().x, -0.25f, easeT);
			enemy->SetRotation(currentRot);
			enemy->SetObjRotation(currentRot); 
		}
		else {
			phase_ = Phase::kShaking;
			timer_ = 0;
			// エフェクト開始
			if (auto* effect = enemy->GetPowerUpEffect()) {
				effect->SetActive(true);
			}
			// JSONを再ロードする (以前の状態に戻す)
			GlobalVariables::GetInstance()->LoadFile("EnemyPowerUp");
		}
		break;

	case Phase::kShaking:
		if (timer_ <= kShakeTime) {
			if (player) {
				// 以前のスピード (0.8f) と振幅 (0.12f) に戻す
				float shakeSpeed = 0.8f; 
				float shakeAmount = 0.12f;
				
				float shakeOffsetX = std::sin(static_cast<float>(timer_) * shakeSpeed) * shakeAmount;
				float shakeOffsetZ = std::cos(static_cast<float>(timer_) * shakeSpeed * 1.5f) * shakeAmount;

				Vector3 shakenPos = startPos_;
				shakenPos.x += shakeOffsetX;
				shakenPos.z += shakeOffsetZ;
				enemy->SetTranslation(shakenPos);

				if (auto* effect = enemy->GetPowerUpEffect()) {
					effect->SetPosition(enemy->GetCenterPosition());
				}
			}
		}
		else {
			// エフェクト停止
			if (auto* effect = enemy->GetPowerUpEffect()) {
				effect->SetActive(false);
			}
			phase_ = Phase::kFacingForward;
			timer_ = 0;
		}
		break;

	case Phase::kFacingForward:
		if (timer_ <= 15) {
			float t = static_cast<float>(timer_) / 15.0f;
			float easeT = 0.5f * (1.0f - std::cos(t * std::numbers::pi_v<float>));
			
			Vector3 currentRot = enemy->GetWorldRotation();
			currentRot.x = ManualLerp(-0.25f, enemy->GetOriginalRotation().x, easeT);
			enemy->SetRotation(currentRot);
			enemy->SetObjRotation(currentRot);
		}
		else {
			phase_ = Phase::kSlowLanding;
			timer_ = 0;
			startPos_ = enemy->GetCenterPosition();
			targetPos_ = { 0.0f, 0.0f, 15.0f }; // 地面の目標座標
		}
		break;

	case Phase::kSlowLanding:
		if (timer_ <= kSlowLandingTime) {
			float t = static_cast<float>(timer_) / static_cast<float>(kSlowLandingTime);
			float invT = 1.0f - t;
			float easeT = 1.0f - (invT * invT * invT);
			enemy->SetTranslation(ManualVector3Lerp(startPos_, targetPos_, easeT));
		}
		else {
			enemy->SetIsPhase2(true);
			enemy->SetIsInvincible(false);
			
			Vector3 finalRot = enemy->GetWorldRotation();
			finalRot.x = enemy->GetOriginalRotation().x;
			finalRot.z = enemy->GetOriginalRotation().z;
			enemy->SetRotation(finalRot);
			enemy->SetObjRotation(enemy->GetOriginalRotation());
			
			return std::make_unique<EnemyStatePlaying>();
		}
		break;
	}

	return nullptr;
}

void EnemyStateTransformation::Exit(Enemy* enemy)
{
	enemy->SetIsInvincible(false);
}
