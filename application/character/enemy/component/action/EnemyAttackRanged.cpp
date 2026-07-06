#include "EnemyAttackRanged.h"
#include "Enemy.h"
#include "Player.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "ObjColor.h"
#include <cmath>

using namespace Engine;
const std::string EnemyAttackRanged::kGroupName_ = "EnemyAttackRanged";

EnemyAttackRanged::EnemyAttackRanged()
{
	variables_ = GlobalVariables::GetInstance();

	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	// タイマー系
	variables_->AddItem(kGroupName_, "Preparation Time", static_cast<int32_t>(kPreparationTime_));
	variables_->AddItem(kGroupName_, "Recovery Time", static_cast<int32_t>(kRecoveryTime_));
	variables_->AddItem(kGroupName_, "Attack Interval", static_cast<int32_t>(kAttackInterval_));
	variables_->AddItem(kGroupName_, "Attack Count", static_cast<int32_t>(kAttackCount_));
	variables_->AddItem(kGroupName_, "Warning Duration", static_cast<int32_t>(kWarningDuration_));
	variables_->AddItem(kGroupName_, "Spike Rise Duration", static_cast<int32_t>(kSpikeRiseDuration_));
	variables_->AddItem(kGroupName_, "Spike Hold Duration", static_cast<int32_t>(kSpikeHoldDuration_));
	variables_->AddItem(kGroupName_, "Spike Fall Duration", static_cast<int32_t>(kSpikeFallDuration_));
	// 挙動
	variables_->AddItem(kGroupName_, "Spike Max Height", kSpikeMaxHeight_);
	variables_->AddItem(kGroupName_, "Warning Circle Radius", kWarningCircleRadius_);
	variables_->AddItem(kGroupName_, "Preparation Tilt Angle", kPreparationTiltAngle_);
	variables_->AddItem(kGroupName_, "Ranged Damage", kRangedDamage_);
}
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

	ApplyVariables();

	// フェーズ2の調整（元値をベースに計算する）
	if (enemy->GetIsPhase2()) {
		kWarningDuration_ /= 2;
		kAttackCount_ *= 2;
	}

	// 警告時間が0にならないように最低値を保証
	if (kWarningDuration_ == 0) kWarningDuration_ = 1;

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	preparationTimer_ = 0;
	attackTimer_ = 0;
	recoveryTimer_ = 0;
	attackPhase_ = 0;
	attackInstances_.clear();

	// 元の回転を保存
	originalRotation_ = enemy->GetCenterRotation();
	// シェイクのベース座標を保存
	shakeBasePosition_ = enemy->GetCenterPosition();

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

	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	if (preparationTimer_ <= kPreparationTime_) {
		float tiltProgress = static_cast<float>(preparationTimer_) / static_cast<float>(kPreparationTime_);
		float tiltAmount = tiltProgress * kPreparationTiltAngle_;

		Vector3 baseRotation = enemy->GetWorldRotation();
		Vector3 objRotation = baseRotation;
		objRotation.x = originalRotation_.x - tiltAmount;
		enemy->SetRotation(objRotation);
		enemy->SetObjRotation(objRotation);
	}

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

	// 攻撃フェーズ中のシェイク効果（ベース座標を基準にする）
	float shakeOffsetX = sin(attackTimer_ * kShakeSpeed_) * kShakeAmount_;
	float shakeOffsetZ = cos(attackTimer_ * kShakeSpeed_ * kShakeSpeedZScale_) * kShakeAmount_;

	Vector3 shakenPosition = shakeBasePosition_;
	shakenPosition.x += shakeOffsetX;
	shakenPosition.z += shakeOffsetZ;
	enemy->SetWorldPosition(shakenPosition);

	// 姿勢の維持（Preparationで設定した回転を維持）
	Vector3 currentRot = enemy->GetWorldRotation();
	currentRot.x = originalRotation_.x - kPreparationTiltAngle_;
	enemy->SetRotation(currentRot);
	enemy->SetObjRotation(currentRot);

	// ジオグリフ生成フェーズ
	if (attackTimer_ % kAttackInterval_ == 0 && attackPhase_ < kAttackCount_) {
		RangedAttackInstance newAttack;

		// プレイヤーの現在位置を取得(各トゲごとに更新)
		Vector3 currentPlayerPos = Vector3(0.0f, 0.0f, 0.0f);
		if (player != nullptr) {
			currentPlayerPos = player->GetCenterPosition();
		}

		// プレイヤーの足元に出現 (中心座標から下げて地面に合わせる)
		newAttack.position = currentPlayerPos;
		newAttack.position.y -= 1.0f; // 腰から足元へのオフセット
		newAttack.position.y += kSpikeGroundOffsetY_; 
		newAttack.isWarningActive = true;

		// 警告円の初期化(輪郭)
		newAttack.warningOutline = std::make_unique<Object3d>();
		newAttack.warningOutline->Initialize("enemy/effect/warningOutLine.obj");
		newAttack.warningOutline->SetSize({ kWarningCircleRadius_, 1.0f, kWarningCircleRadius_ });
		newAttack.warningOutline->SetRotation({ 0.0f, 0.0f, 0.0f }); // モデルが既に水平なので0

		// 警告円の初期化(塗りつぶし)
		newAttack.warningFill = std::make_unique<Object3d>();
		newAttack.warningFill->Initialize("enemy/effect/warningFill.obj");
		newAttack.warningFill->SetSize({ 0.0f, 1.0f, 0.0f }); // 最初はサイズ0
		newAttack.warningFill->SetRotation({ 0.0f, 0.0f, 0.0f });

		// トゲモデルの初期化(個別に新しいオブジェクトを作成)
		newAttack.spike = std::make_unique<Object3d>();
		newAttack.spike->Initialize("Enemy/Cube.obj");
		newAttack.spike->SetSize({ 1.0f, 0.0f, 1.0f }); // 初期は高さ0

		// トランスフォームの初期化
		newAttack.warningTransform.Initialize();
		newAttack.warningFillTransform.Initialize();
		newAttack.spikeTransform.Initialize();

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

	enemy->SetVelocity(Vector3(0.0f, 0.0f, 0.0f));

	float recoveryProgress = static_cast<float>(recoveryTimer_) / static_cast<float>(kRecoveryTime_);
	float currentTilt = -kPreparationTiltAngle_ * (1.0f - recoveryProgress);

	Vector3 baseRotation = enemy->GetWorldRotation();
	Vector3 objRotation = baseRotation;
	objRotation.x = originalRotation_.x + currentTilt;
	enemy->SetRotation(objRotation);

	if (recoveryTimer_ >= kRecoveryTime_) {
		Vector3 currentRotation = enemy->GetWorldRotation();
		currentRotation.x = originalRotation_.x;
		enemy->SetRotation(currentRotation);

		phase_ = Phase::kNone;
		isComplete_ = true;

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

			// 塗りつぶし円のサイズ更新
			if (attack.warningFill) {
				float progress = static_cast<float>(attack.warningTimer) / static_cast<float>(kWarningDuration_);
				float currentRadius = progress * kWarningCircleRadius_;
				attack.warningFill->SetSize({ currentRadius, 1.0f, currentRadius });
			}

			if (attack.warningTimer >= kWarningDuration_) {
				attack.isWarningActive = false;
				attack.isSpikeActive = true;
				attack.spikeTimer = 0;
			}
		}

		// トゲの更新
		if (attack.isSpikeActive) {
			attack.spikeTimer++;

			if (attack.spikeTimer < kSpikeRiseDuration_) {
				float riseProgress = static_cast<float>(attack.spikeTimer) / static_cast<float>(kSpikeRiseDuration_);
				attack.spikeHeight = riseProgress * kSpikeMaxHeight_;
			}
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_) {
				attack.spikeHeight = kSpikeMaxHeight_;
			}
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_ + kSpikeFallDuration_) {
				uint32_t fallTimer = attack.spikeTimer - (kSpikeRiseDuration_ + kSpikeHoldDuration_);
				float fallProgress = static_cast<float>(fallTimer) / static_cast<float>(kSpikeFallDuration_);
				attack.spikeHeight = kSpikeMaxHeight_ * (1.0f - fallProgress);
			}
			else {
				attack.isSpikeActive = false;
				attack.spikeHeight = 0.0f;
			}

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

	for (auto& attack : attackInstances_) {
		if (!attack.isSpikeActive) continue;
		if (attack.hasHitPlayer) continue;          // このトゲでは既に当たり判定済み

		if (attack.spikeHeight < kSpikeMaxHeight_ * kSpikeHitHeightRatio_) continue;

		float distanceXZ = std::sqrt(
			std::pow(playerPos.x - attack.position.x, 2.0f) +
			std::pow(playerPos.z - attack.position.z, 2.0f)
		);

		if (distanceXZ <= kWarningCircleRadius_ &&
			playerPos.y <= attack.position.y + kSpikeMaxHeight_) {
			if (!player->IsDodging()) {
				player->ApplyDamage(static_cast<uint32_t>(kRangedDamage_), attack.position);
				attack.hasHitPlayer = true;         // このトゲからのダメージは1回だけ
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
		if (attack.warningOutline) {
			attack.warningTransform.translation_ = attack.position;
			attack.warningTransform.rotation_ = attack.warningOutline->GetRotation();
			attack.warningTransform.scale_ = attack.warningOutline->GetSize();
			attack.warningTransform.UpdateMatrix();
			attack.warningOutline->Update(attack.warningTransform, vp);
		}

		if (attack.warningFill) {
			attack.warningFillTransform.translation_ = attack.position;
			attack.warningFillTransform.rotation_ = attack.warningFill->GetRotation();
			attack.warningFillTransform.scale_ = attack.warningFill->GetSize();
			attack.warningFillTransform.UpdateMatrix();
			attack.warningFill->Update(attack.warningFillTransform, vp);
		}

		if (attack.spike) {
			attack.spikeTransform.translation_ = attack.position;
			attack.spikeTransform.translation_.y += attack.spikeHeight * kSpikeCenterOffsetScale_;
			attack.spikeTransform.scale_ = attack.spike->GetSize();
			attack.spikeTransform.UpdateMatrix();
			attack.spike->Update(attack.spikeTransform, vp);
		}
	}
}

void EnemyAttackRanged::Draw(const ViewProjection& viewProjection)
{
	for (auto& attack : attackInstances_) {
		if (attack.isWarningActive) {
			ObjColor redColor;
			redColor.Initialize();
			redColor.SetColor(Vector4(1.0f, 0.0f, 0.0f, 0.6f));
			redColor.TransferMatrix();

			if (attack.warningOutline) {
				attack.warningTransform.UpdateMatrix();
				attack.warningOutline->Draw(attack.warningTransform, viewProjection, &redColor);
			}

			if (attack.warningFill) {
				attack.warningFillTransform.UpdateMatrix();
				attack.warningFill->Draw(attack.warningFillTransform, viewProjection, &redColor);
			}

		}

		if (attack.isSpikeActive && attack.spike) {
			attack.spikeTransform.UpdateMatrix();
			attack.spike->Draw(attack.spikeTransform, viewProjection);
		}
	}
}

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void EnemyAttackRanged::ApplyVariables()
{
	kPreparationTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Preparation Time"));
	kRecoveryTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recovery Time"));
	kAttackInterval_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Attack Interval"));
	kAttackCount_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Attack Count"));
	kWarningDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Warning Duration"));
	kSpikeRiseDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Spike Rise Duration"));
	kSpikeHoldDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Spike Hold Duration"));
	kSpikeFallDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Spike Fall Duration"));
	kSpikeMaxHeight_ = variables_->GetFloatValue(kGroupName_, "Spike Max Height");
	kWarningCircleRadius_ = variables_->GetFloatValue(kGroupName_, "Warning Circle Radius");
	kPreparationTiltAngle_ = variables_->GetFloatValue(kGroupName_, "Preparation Tilt Angle");
	kRangedDamage_ = variables_->GetIntValue(kGroupName_, "Ranged Damage");
}