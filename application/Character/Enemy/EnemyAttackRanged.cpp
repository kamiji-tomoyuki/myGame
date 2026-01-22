#include "EnemyAttackRanged.h"
#include "Enemy.h"
#include "Player.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "ObjColor.h"
#include <cmath>

EnemyAttackRanged::EnemyAttackRanged() = default;
EnemyAttackRanged::~EnemyAttackRanged() = default;

void EnemyAttackRanged::Initialize()
{
	phase_ = Phase::kNone;
	isComplete_ = false;
	preparationTimer_ = 0;
	attackTimer_ = 0;
	recoveryTimer_ = 0;
	attackPhase_ = 0;
	attackInstances_.clear();
}

void EnemyAttackRanged::Start(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	preparationTimer_ = 0;
	attackTimer_ = 0;
	recoveryTimer_ = 0;
	attackPhase_ = 0;
	attackInstances_.clear();

	// 元の回転を保存
	originalRotation_ = enemy->GetCenterRotation();

	// プレイヤーの方向を向く
	Vector3 targetPos = player->GetCenterPosition();
	Vector3 currentPos = enemy->GetCenterPosition();
	Vector3 direction = (targetPos - currentPos).Normalize();

	float targetRotationY = std::atan2(direction.x, direction.z);
	enemy->SetRotationY(targetRotationY);

	// 移動を停止(硬直状態)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
}

void EnemyAttackRanged::Update(Enemy* enemy, Player* player)
{
	if (enemy == nullptr) return;

	switch (phase_) {
	case Phase::kPreparation:
		UpdatePreparation(enemy);
		break;

	case Phase::kAttacking:
		UpdateAttacking(enemy, player);
		break;

	case Phase::kRecovery:
		UpdateRecovery(enemy);
		break;
	}
}

void EnemyAttackRanged::UpdatePreparation(Enemy* enemy)
{
	preparationTimer_++;

	// 硬直状態を維持(移動しない)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// 後ろに傾く予備動作アニメーション(詠唱ポーズ)
	if (preparationTimer_ < kPreparationTime_) {
		float tiltProgress = static_cast<float>(preparationTimer_) / kPreparationTime_;
		float tiltAmount = tiltProgress * kPreparationTiltAngle_;

		Vector3 baseRotation = enemy->GetWorldRotation();
		Vector3 objRotation = baseRotation;
		objRotation.x = originalRotation_.x - tiltAmount;  // 後ろに傾く(マイナス方向)
		enemy->SetRotation(objRotation);
	}

	// 予備動作終了
	if (preparationTimer_ >= kPreparationTime_) {
		phase_ = Phase::kAttacking;
		attackTimer_ = 0;
	}
}

void EnemyAttackRanged::UpdateAttacking(Enemy* enemy, Player* player)
{
	attackTimer_++;

	// 硬直状態を維持(移動しない)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// 攻撃フェーズ中のシェイク効果
	static constexpr float kShakeAmount = 0.04f;
	static constexpr float kShakeSpeed = 0.7f;

	float shakeOffsetX = sin(attackTimer_ * kShakeSpeed) * kShakeAmount;
	float shakeOffsetZ = cos(attackTimer_ * kShakeSpeed * 1.3f) * kShakeAmount;

	Vector3 basePosition = enemy->GetCenterPosition();
	Vector3 shakenPosition = basePosition;
	shakenPosition.x += shakeOffsetX;
	shakenPosition.z += shakeOffsetZ;
	enemy->SetWorldPosition(shakenPosition);

	// ジオグリフ生成フェーズ
	if (attackTimer_ % kAttackInterval_ == 0 && attackPhase_ < kAttackCount_) {
		RangedAttackInstance newAttack;

		// プレイヤーの現在位置を取得(各トゲごとに更新)
		Vector3 currentPlayerPos = Vector3(0.0f, 0.0f, 0.0f);
		if (player != nullptr) {
			currentPlayerPos = player->GetCenterPosition();
		}

		// プレイヤーの足元に出現
		newAttack.position = currentPlayerPos;
		newAttack.position.y = 0.1f; // 地面の高さ
		newAttack.isWarningActive = true;

		// 警告円の初期化(個別に新しいオブジェクトを作成)
		newAttack.warningCircle = std::make_unique<Object3d>();
		newAttack.warningCircle->Initialize("debug/ground.obj");
		newAttack.warningCircle->SetSize({ kWarningCircleRadius_, 1.0f, kWarningCircleRadius_ });
		newAttack.warningCircle->SetRotation({ 1.57f, 0.0f, 0.0f }); // X軸90度回転で地面に

		// トゲモデルの初期化(個別に新しいオブジェクトを作成)
		newAttack.spike = std::make_unique<Object3d>();
		newAttack.spike->Initialize("Enemy/Cube.obj");
		newAttack.spike->SetSize({ 1.0f, 0.0f, 1.0f }); // 初期は高さ0

		attackInstances_.push_back(std::move(newAttack));
		attackPhase_++;
	}

	// 既存の攻撃インスタンスを更新
	UpdateAttackInstances(player);

	// 全ての攻撃が終了したかチェック
	bool allFinished = attackPhase_ >= kAttackCount_;
	if (allFinished) {
		bool anyActive = false;
		for (const auto& attack : attackInstances_) {
			if (attack.isWarningActive || attack.isSpikeActive) {
				anyActive = true;
				break;
			}
		}
		if (!anyActive) {
			// 姿勢回復フェーズに移行
			phase_ = Phase::kRecovery;
			recoveryTimer_ = 0;
		}
	}
}

void EnemyAttackRanged::UpdateRecovery(Enemy* enemy)
{
	recoveryTimer_++;

	// 硬直状態を維持(移動しない)
	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	// 傾きを徐々に元に戻す補間処理
	float recoveryProgress = static_cast<float>(recoveryTimer_) / kRecoveryTime_;
	float currentTilt = -kPreparationTiltAngle_ * (1.0f - recoveryProgress);

	Vector3 baseRotation = enemy->GetWorldRotation();
	Vector3 objRotation = baseRotation;
	objRotation.x = originalRotation_.x + currentTilt;
	enemy->SetRotation(objRotation);

	// 回復時間が終了したら通常状態に
	if (recoveryTimer_ >= kRecoveryTime_) {
		// 完全に元の姿勢に戻す
		Vector3 currentRotation = enemy->GetWorldRotation();
		currentRotation.x = originalRotation_.x;
		enemy->SetRotation(currentRotation);

		// 攻撃完了
		phase_ = Phase::kNone;
		isComplete_ = true;

		// 攻撃データをクリア
		attackInstances_.clear();
		attackPhase_ = 0;
		attackTimer_ = 0;
	}
}

void EnemyAttackRanged::UpdateAttackInstances(Player* player)
{
	for (auto& attack : attackInstances_) {
		// 警告円の更新
		if (attack.isWarningActive) {
			attack.warningTimer++;

			if (attack.warningTimer >= kWarningDuration_) {
				attack.isWarningActive = false;
				attack.isSpikeActive = true;
				attack.spikeTimer = 0;
			}
		}

		// トゲの更新
		if (attack.isSpikeActive) {
			attack.spikeTimer++;

			// トゲの上昇アニメーション
			if (attack.spikeTimer < kSpikeRiseDuration_) {
				// 上昇フェーズ
				float riseProgress = static_cast<float>(attack.spikeTimer) / kSpikeRiseDuration_;
				attack.spikeHeight = riseProgress * kSpikeMaxHeight_;
			}
			// トゲの持続
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_) {
				// 最大高さを維持
				attack.spikeHeight = kSpikeMaxHeight_;
			}
			// トゲの下降アニメーション
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_ + kSpikeFallDuration_) {
				// 下降フェーズ
				uint32_t fallTimer = attack.spikeTimer - (kSpikeRiseDuration_ + kSpikeHoldDuration_);
				float fallProgress = static_cast<float>(fallTimer) / kSpikeFallDuration_;
				attack.spikeHeight = kSpikeMaxHeight_ * (1.0f - fallProgress);
			}
			else {
				// 完全に引っ込んだら非アクティブに
				attack.isSpikeActive = false;
				attack.spikeHeight = 0.0f;
			}

			// トゲのサイズ更新
			attack.spike->SetSize({ 1.0f, attack.spikeHeight, 1.0f });
		}
	}

	// 当たり判定チェック
	CheckCollision(player);
}

void EnemyAttackRanged::CheckCollision(Player* player)
{
	if (player == nullptr) return;

	Vector3 playerPos = player->GetCenterPosition();

	for (const auto& attack : attackInstances_) {
		if (!attack.isSpikeActive) continue;

		// トゲが十分に伸びている場合のみ判定
		if (attack.spikeHeight < kSpikeMaxHeight_ * 0.5f) continue;

		// 円形の当たり判定
		float distanceXZ = std::sqrt(
			std::pow(playerPos.x - attack.position.x, 2.0f) +
			std::pow(playerPos.z - attack.position.z, 2.0f)
		);

		// プレイヤーがトゲの範囲内にいる場合
		if (distanceXZ <= kWarningCircleRadius_ &&
			playerPos.y <= attack.position.y + kSpikeMaxHeight_) {
			// プレイヤーにダメージを与える処理
			// プレイヤーが回避中でない場合のみダメージ
			if (!player->IsDodging()) {
				//player->TakeDamage(attack.position);
			}
		}
	}
}

void EnemyAttackRanged::Interrupt()
{
	phase_ = Phase::kNone;
	isComplete_ = true;
	attackInstances_.clear();
	attackPhase_ = 0;
	attackTimer_ = 0;
}

void EnemyAttackRanged::UpdateViewProjection(const ViewProjection& vp)
{
	vp_ = &vp;

	// 攻撃インスタンスのモデル更新
	for (auto& attack : attackInstances_) {
		if (attack.warningCircle) {
			WorldTransform warningTransform;
			warningTransform.Initialize();
			warningTransform.translation_ = attack.position;
			warningTransform.rotation_ = attack.warningCircle->GetRotation();
			warningTransform.scale_ = attack.warningCircle->GetSize();
			warningTransform.UpdateMatrix();
			attack.warningCircle->Update(warningTransform, vp);
		}

		if (attack.spike) {
			WorldTransform spikeTransform;
			spikeTransform.Initialize();
			spikeTransform.translation_ = attack.position;
			spikeTransform.translation_.y += attack.spikeHeight * 0.5f;
			spikeTransform.scale_ = attack.spike->GetSize();
			spikeTransform.UpdateMatrix();
			attack.spike->Update(spikeTransform, vp);
		}
	}
}

void EnemyAttackRanged::Draw(const ViewProjection& viewProjection)
{
	for (const auto& attack : attackInstances_) {
		if (attack.isWarningActive && attack.warningCircle) {
			WorldTransform warningTransform;
			warningTransform.Initialize();
			warningTransform.translation_ = attack.position;
			warningTransform.rotation_ = attack.warningCircle->GetRotation();
			warningTransform.scale_ = attack.warningCircle->GetSize();
			warningTransform.UpdateMatrix();

			// 赤色で描画
			ObjColor redColor;
			redColor.Initialize();
			redColor.SetColor(Vector4(1.0f, 0.0f, 0.0f, 0.6f));
			redColor.TransferMatrix();

			//attack.warningCircle->Draw(warningTransform, viewProjection, &redColor);
		}

		if (attack.isSpikeActive && attack.spike) {
			WorldTransform spikeTransform;
			spikeTransform.Initialize();
			spikeTransform.translation_ = attack.position;
			spikeTransform.translation_.y += attack.spikeHeight * 0.5f;
			spikeTransform.scale_ = attack.spike->GetSize();
			spikeTransform.UpdateMatrix();

			attack.spike->Draw(spikeTransform, viewProjection);
		}
	}
}