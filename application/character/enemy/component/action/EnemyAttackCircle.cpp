#include "EnemyAttackCircle.h"
#include "Enemy.h"
#include "Player.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "ObjColor.h"
#include "myMath.h"
#include <cmath>
#include <algorithm>

const std::string EnemyAttackCircle::kGroupName_ = "EnemyAttackCircle";

EnemyAttackCircle::EnemyAttackCircle()
{
	variables_ = GlobalVariables::GetInstance();
	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	variables_->AddItem(kGroupName_, "Prep Time", static_cast<int32_t>(kPrepTime_));
	variables_->AddItem(kGroupName_, "Recovery Time", static_cast<int32_t>(kRecoveryTime_));
	variables_->AddItem(kGroupName_, "Attack Radius", 15.0f);
	variables_->AddItem(kGroupName_, "Jump Height", kJumpHeight_);
	variables_->AddItem(kGroupName_, "Gravity", kGravity_);
	variables_->AddItem(kGroupName_, "Damage", kDamage_);
	variables_->AddItem(kGroupName_, "Tilt Angle", kTiltAngle_);
	variables_->AddItem(kGroupName_, "Shake Amount", 0.35f);

	warningOutline_ = std::make_unique<Object3d>();
	warningOutline_->Initialize("enemy/effect/warningOutLine.obj");
	warningFill_ = std::make_unique<Object3d>();
	warningFill_->Initialize("enemy/effect/warningFill.obj");

	warningTransform_.Initialize();
	warningFillTransform_.Initialize();
}

EnemyAttackCircle::~EnemyAttackCircle() = default;

void EnemyAttackCircle::Initialize()
{
	phase_ = Phase::kNone;
	isComplete_ = false;
	timer_ = 0;
	isWarningActive_ = false;
}

void EnemyAttackCircle::Start(Enemy* enemy, Player* player)
{
	if (!enemy || !player) return;

	ApplyVariables();

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	timer_ = 0;

	startPosition_ = enemy->GetWorldPosition();
	baseRotation_ = enemy->GetWorldRotation();

	// 距離判定
	Vector3 toPlayer = player->GetCenterPosition() - enemy->GetCenterPosition();
	toPlayer.y = 0.0f;
	float dist = toPlayer.Length();

	if (dist < 15.0f) {
		isLongRange_ = false;
		targetPosition_ = enemy->GetWorldPosition();
	}
	else {
		isLongRange_ = true;
		targetPosition_ = player->GetCenterPosition();
		targetPosition_.y = startPosition_.y; // 高さ合わせ
	}

	isWarningActive_ = true;
	warningOutline_->SetSize({ kAttackRadius_, 1.0f, kAttackRadius_ });
	warningFill_->SetSize({ 0.0f, 1.0f, 0.0f });

	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void EnemyAttackCircle::Update(Enemy* enemy, Player* player)
{
	if (!enemy) return;

	switch (phase_) {
	case Phase::kPreparation:
		UpdatePreparation(enemy, player);
		break;
	case Phase::kJump:
		UpdateJump(enemy, player);
		break;
	case Phase::kRecovery:
		UpdateRecovery(enemy);
		break;
	}
}

void EnemyAttackCircle::UpdatePreparation(Enemy* enemy, Player* player)
{
	timer_++;

	// 警告表示更新
	float progress = std::min(1.0f, static_cast<float>(timer_) / kPrepTime_);
	warningFill_->SetSize({ progress * kAttackRadius_, 1.0f, progress * kAttackRadius_ });

	// 前に傾く
	Vector3 rot = baseRotation_;
	rot.x += kTiltAngle_ * progress; // 前傾
	enemy->SetRotation(rot);
	enemy->SetObjRotation(rot);

	// シェイク値を更新して保持
	currentShake_.x = ((rand() % 100) / 100.0f - 0.5f) * kShakeAmount_;
	currentShake_.z = ((rand() % 100) / 100.0f - 0.5f) * kShakeAmount_;
	currentShake_.y = 0.0f;

	enemy->SetWorldPosition({ startPosition_.x + currentShake_.x, startPosition_.y, startPosition_.z + currentShake_.z });

	if (timer_ >= kPrepTime_) {
		phase_ = Phase::kJump;
		timer_ = 0;
		currentShake_ = { 0.0f, 0.0f, 0.0f };
		enemy->SetWorldPosition(startPosition_);
		
		// ジャンプ初期化
		if (isLongRange_) {
			// 放物線ジャンプの計算 (簡易)
			// 到達時間を計算。滞空時間を 60 フレームと仮定
			float airTime = 60.0f;
			jumpVelocityY_ = (kGravity_ * airTime) * 0.5f; // 最高点に達するまでの速度
		}
		else {
			jumpVelocityY_ = 0.6f; // その場ジャンプ
		}
	}
}

void EnemyAttackCircle::UpdateJump(Enemy* enemy, Player* player)
{
	timer_++;

	Vector3 pos = enemy->GetWorldPosition();
	
	if (isLongRange_) {
		// ターゲットへ移動
		float airTime = 60.0f;
		float progress = std::min(1.0f, static_cast<float>(timer_) / airTime);
		
		Vector3 currentPos = enemy->GetWorldPosition();
		Vector3 nextXZ = startPosition_ + (targetPosition_ - startPosition_) * progress;
		
		pos.x = nextXZ.x;
		pos.z = nextXZ.z;
		pos.y += jumpVelocityY_;
		jumpVelocityY_ -= kGravity_;

		if (progress >= 1.0f && pos.y <= targetPosition_.y) {
			pos.y = targetPosition_.y;
			enemy->SetWorldPosition(pos);
			CheckCollision(enemy, player);
			phase_ = Phase::kRecovery;
			timer_ = 0;
			isWarningActive_ = false;
		}
	}
	else {
		pos.y += jumpVelocityY_;
		jumpVelocityY_ -= kGravity_;

		if (jumpVelocityY_ < 0.0f && pos.y <= startPosition_.y) {
			pos.y = startPosition_.y;
			enemy->SetWorldPosition(pos);
			CheckCollision(enemy, player);
			phase_ = Phase::kRecovery;
			timer_ = 0;
			isWarningActive_ = false;
		}
	}

	enemy->SetWorldPosition(pos);
}

void EnemyAttackCircle::UpdateRecovery(Enemy* enemy)
{
	timer_++;

	float progress = std::min(1.0f, static_cast<float>(timer_) / kRecoveryTime_);
	Vector3 rot = enemy->GetWorldRotation();
	rot.x = baseRotation_.x + kTiltAngle_ * (1.0f - progress); // 復帰
	enemy->SetRotation(rot);
	enemy->SetObjRotation(rot);

	if (timer_ >= kRecoveryTime_) {
		enemy->SetRotation(baseRotation_);
		enemy->SetObjRotation(baseRotation_);
		isComplete_ = true;
		phase_ = Phase::kNone;
	}
}

void EnemyAttackCircle::CheckCollision(Enemy* enemy, Player* player)
{
	if (!player) return;

	Vector3 center = isLongRange_ ? targetPosition_ : startPosition_;
	Vector3 playerPos = player->GetCenterPosition();
	
	Vector3 to_player = playerPos - center;
	to_player.y = 0.0f;
	
	if (to_player.Length() <= kAttackRadius_) {
		if (!player->IsDodging()) {
			player->ApplyDamage(static_cast<uint32_t>(kDamage_), center);
		}
	}
}

void EnemyAttackCircle::UpdateViewProjection(const ViewProjection& vp)
{
	vp_ = &vp;
	if (isWarningActive_) {
		// 地面より少し浮かせ、被りを防ぐ (Ground: -2.5f)
		float groundY = -2.5f + 0.1f; 

		Vector3 pos = targetPosition_;
		// 警告円はシェイクさせず、固定座標に描画

		warningTransform_.translation_ = pos;
		warningTransform_.translation_.y = groundY; 
		warningTransform_.scale_ = warningOutline_->GetSize();
		warningTransform_.UpdateMatrix();
		warningOutline_->Update(warningTransform_, vp);

		warningFillTransform_.translation_ = pos;
		warningFillTransform_.translation_.y = groundY + 0.01f; // 輪郭よりわずかに上に
		warningFillTransform_.scale_ = warningFill_->GetSize(); // アニメーション中のサイズを適用
		warningFillTransform_.UpdateMatrix();
		warningFill_->Update(warningFillTransform_, vp);
	}
}

void EnemyAttackCircle::Draw(const ViewProjection& viewProjection)
{
	if (isWarningActive_) {
		ObjColor redColor;
		redColor.Initialize();
		redColor.SetColor({ 1.0f, 0.0f, 0.0f, 0.6f });
		redColor.TransferMatrix();

		warningOutline_->Draw(warningTransform_, viewProjection, &redColor);
		warningFill_->Draw(warningFillTransform_, viewProjection, &redColor);
	}
}

void EnemyAttackCircle::Interrupt()
{
	phase_ = Phase::kNone;
	isComplete_ = true;
	isWarningActive_ = false;
}

void EnemyAttackCircle::ApplyVariables()
{
	kPrepTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Prep Time"));
	kRecoveryTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recovery Time"));
	kAttackRadius_ = variables_->GetFloatValue(kGroupName_, "Attack Radius");
	kJumpHeight_ = variables_->GetFloatValue(kGroupName_, "Jump Height");
	kGravity_ = variables_->GetFloatValue(kGroupName_, "Gravity");
	kDamage_ = variables_->GetIntValue(kGroupName_, "Damage");
	kTiltAngle_ = variables_->GetFloatValue(kGroupName_, "Tilt Angle");
	kShakeAmount_ = variables_->GetFloatValue(kGroupName_, "Shake Amount");
}
