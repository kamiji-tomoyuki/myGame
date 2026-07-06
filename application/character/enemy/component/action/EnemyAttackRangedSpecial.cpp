#include "EnemyAttackRangedSpecial.h"
#include "Enemy.h"
#include "Player.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "myMath.h"
#include <cmath>
#include <algorithm>

using namespace Engine;
const std::string EnemyAttackRangedSpecial::kGroupName_ = "EnemyAttackRangedSpecial";

EnemyAttackRangedSpecial::EnemyAttackRangedSpecial()
{
	variables_ = GlobalVariables::GetInstance();
	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}

	variables_->AddItem(kGroupName_, "Prep Time", static_cast<int32_t>(kPrepTime_));
	variables_->AddItem(kGroupName_, "Recovery Time", static_cast<int32_t>(kRecoveryTime_));
	variables_->AddItem(kGroupName_, "Launch Duration", static_cast<int32_t>(kLaunchDuration_));
	variables_->AddItem(kGroupName_, "Spike Speed", kSpikeSpeed_);
	variables_->AddItem(kGroupName_, "Homing Strength", kHomingStrength_);
	variables_->AddItem(kGroupName_, "Spike Life Time", static_cast<int32_t>(kSpikeLifeTime_));
	variables_->AddItem(kGroupName_, "Spike Count", static_cast<int32_t>(kSpikeCount_));
	variables_->AddItem(kGroupName_, "Damage", kDamage_);
}

EnemyAttackRangedSpecial::~EnemyAttackRangedSpecial() = default;

void EnemyAttackRangedSpecial::Initialize()
{
	phase_ = Phase::kNone;
	isComplete_ = false;
	timer_ = 0;
	launchTimer_ = 0;
	projectiles_.clear();
}

void EnemyAttackRangedSpecial::Start(Enemy* enemy, Player* player)
{
	if (!enemy || !player) return;

	ApplyVariables();

	phase_ = Phase::kPreparation;
	isComplete_ = false;
	timer_ = 0;
	launchTimer_ = 0;
	projectiles_.clear();

	baseRotation_ = enemy->GetWorldRotation();
	basePosition_ = enemy->GetWorldPosition();

	// プレイヤーの方向を向く
	Vector3 toPlayer = player->GetCenterPosition() - enemy->GetCenterPosition();
	float targetRotY = std::atan2(toPlayer.x, toPlayer.z);
	enemy->SetRotationY(targetRotY);
	baseRotation_.y = targetRotY;

	enemy->SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void EnemyAttackRangedSpecial::Update(Enemy* enemy, Player* player)
{
	if (!enemy) return;

	switch (phase_) {
	case Phase::kPreparation:
		UpdatePreparation(enemy, player);
		break;
	case Phase::kJump:
		UpdateJump(enemy);
		break;
	case Phase::kLaunch:
		UpdateLaunch(enemy, player);
		break;
	case Phase::kRecovery:
		UpdateRecovery(enemy);
		break;
	}

	UpdateProjectiles(player);
}

void EnemyAttackRangedSpecial::UpdatePreparation(Enemy* enemy, Player* player)
{
	timer_++;

	// 下を向く
	float progress = std::min(1.0f, static_cast<float>(timer_) / (kPrepTime_ * 0.5f));
	Vector3 rot = baseRotation_;
	rot.x += kLookDownAngle_ * progress;
	enemy->SetRotation(rot);
	enemy->SetObjRotation(rot);

	// シェイク (敵本体のみ)
	float shakeX = ((rand() % 100) / 100.0f - 0.5f) * kShakeAmount_;
	float shakeZ = ((rand() % 100) / 100.0f - 0.5f) * kShakeAmount_;
	enemy->SetWorldPosition({ basePosition_.x + shakeX, basePosition_.y, basePosition_.z + shakeZ });

	// トゲの生成と配置 (背後に)
	if (projectiles_.empty()) {
		for (uint32_t i = 0; i < kSpikeCount_; ++i) {
			SpikeProjectile spike;
			spike.model = std::make_unique<Object3d>();
			spike.model->Initialize("Enemy/Cube.obj");
			spike.model->SetSize({ 0.0f, 0.0f, 0.0f }); // 初期スケールは0
			spike.transform.Initialize();
			spike.isActive = true;
			
			// 発射タイミングをさらにばらす
			spike.launchDelay = rand() % kLaunchDuration_;
			// 出現タイミングもばらす (予備動作中にバラバラに出現)
			spike.spawnDelay = rand() % (kPrepTime_ - kSpawnTime_);
			spike.spawnTimer = 0;

			// 初期オフセットをさらに広範囲にばらつきを持たせる (よりバラバラに)
			// 重なりを減らすために、X方向を広げ、ある程度の最小間隔を意識した配置にする
			float spreadX = (static_cast<float>(i) - (kSpikeCount_ - 1) * 0.5f) * 4.0f; // 基本的な横並び
			spreadX += (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 3.0f;     // ランダムな揺らぎ
			
			float spreadY = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 10.0f;
			float spreadZ = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 6.0f;
			spike.startOffset = { spreadX, 4.0f + spreadY, -6.0f + spreadZ };

			projectiles_.push_back(std::move(spike));
		}
	}

	ArrangeSpikes(enemy, player);

	if (timer_ >= kPrepTime_) {
		phase_ = Phase::kJump;
		timer_ = 0;
		jumpVelocityY_ = kJumpPower_;
		enemy->SetWorldPosition(basePosition_);
	}
}

void EnemyAttackRangedSpecial::UpdateJump(Enemy* enemy)
{
	timer_++;

	Vector3 pos = enemy->GetWorldPosition();
	pos.y += jumpVelocityY_;
	jumpVelocityY_ -= kGravity_;
	enemy->SetWorldPosition(pos);

	// トゲは一緒にジャンプさせないように修正

	if (jumpVelocityY_ < 0.0f && pos.y <= basePosition_.y) {
		enemy->SetWorldPosition(basePosition_);
		phase_ = Phase::kLaunch;
		timer_ = 0;
		launchTimer_ = 0;
	}
}

void EnemyAttackRangedSpecial::UpdateLaunch(Enemy* enemy, Player* player)
{
	launchTimer_++;

	bool allLaunched = true;
	if (player) {
		Vector3 playerPos = player->GetCenterPosition();
		for (auto& spike : projectiles_) {
			if (!spike.isLaunched) {
				if (launchTimer_ >= spike.launchDelay) {
					spike.isLaunched = true;
					Vector3 dir = (playerPos - spike.transform.translation_).Normalize();
					spike.velocity = dir * kSpikeSpeed_;
					spike.currentSpeed = kSpikeSpeed_;
				}
				else {
					allLaunched = false;
				}
			}
		}
	}

	if (allLaunched) {
		phase_ = Phase::kRecovery;
		timer_ = 0;
	}
}

void EnemyAttackRangedSpecial::UpdateRecovery(Enemy* enemy)
{
	timer_++;

	float progress = std::min(1.0f, static_cast<float>(timer_) / kRecoveryTime_);
	Vector3 rot = enemy->GetWorldRotation();
	rot.x = baseRotation_.x + kLookDownAngle_ * (1.0f - progress);
	enemy->SetRotation(rot);
	enemy->SetObjRotation(rot);

	if (timer_ >= kRecoveryTime_) {
		enemy->SetRotation(baseRotation_);
		enemy->SetObjRotation(baseRotation_);
		isComplete_ = true;
		phase_ = Phase::kNone;
	}
}

Vector3 LerpVec3(const Vector3& start, const Vector3& end, float t) {
	return start + (end - start) * t;
}

void EnemyAttackRangedSpecial::UpdateProjectiles(Player* player)
{
	if (!player) return;
	Vector3 playerPos = player->GetCenterPosition();

	for (auto it = projectiles_.begin(); it != projectiles_.end(); ) {
		if (it->isLaunched) {
			it->lifeTimer++;

			Vector3 toPlayer = (playerPos - it->transform.translation_).Normalize();
			it->velocity = LerpVec3(it->velocity, toPlayer * it->currentSpeed, kHomingStrength_);
			
			it->transform.translation_ += it->velocity;

			if (it->velocity.Length() > 0.001f) {
				it->transform.rotation_.y = std::atan2(it->velocity.x, it->velocity.z);
				float vXZ = std::sqrt(it->velocity.x * it->velocity.x + it->velocity.z * it->velocity.z);
				it->transform.rotation_.x = std::atan2(-it->velocity.y, vXZ);
			}

			it->transform.UpdateMatrix();
			it->model->Update(it->transform, *vp_);

			float dist = (playerPos - it->transform.translation_).Length();
			if (dist < 1.8f && !it->hasHit) {
				if (!player->IsDodging()) {
					// この攻撃のみ、被弾直後の無敵時間を無視して連続ヒットさせる
					player->ApplyDamageDirect(static_cast<uint32_t>(kDamage_), it->transform.translation_);
					it->hasHit = true; 
				}
			}

			if (it->hasHit || it->lifeTimer >= kSpikeLifeTime_) {
				it = projectiles_.erase(it);
				continue;
			}
		}
		it++;
	}
}

void EnemyAttackRangedSpecial::ArrangeSpikes(Enemy* enemy, Player* player)
{
	if (!enemy || !player) return;

	Vector3 enemyPos = enemy->GetCenterPosition();
	float enemyRotY = enemy->GetWorldRotation().y;

	Matrix4x4 matWorld = MakeRotateYMatrix(enemyRotY);

	for (size_t i = 0; i < projectiles_.size(); ++i) {
		if (projectiles_[i].isLaunched) continue;

		// 出現演出の更新
		if (timer_ >= projectiles_[i].spawnDelay) {
			if (projectiles_[i].spawnTimer < kSpawnTime_) {
				projectiles_[i].spawnTimer++;
			}
		}

		float scaleProgress = static_cast<float>(projectiles_[i].spawnTimer) / kSpawnTime_;
		projectiles_[i].transform.scale_ = { scaleProgress * 0.5f, scaleProgress * 0.5f, scaleProgress * 1.5f };

		// 背後方向（ローカル座標からの変換）
		Vector3 offset = Transformation(projectiles_[i].startOffset, matWorld);
		
		// 浮遊感を個別に持たせる
		offset.y += std::sin(static_cast<float>(i) * 1.5f + timer_ * 0.1f) * 0.4f;

		projectiles_[i].transform.translation_ = enemyPos + offset;
		
		Vector3 toPlayer = (player->GetCenterPosition() - projectiles_[i].transform.translation_).Normalize();
		projectiles_[i].transform.rotation_.y = std::atan2(toPlayer.x, toPlayer.z);
		float vXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
		projectiles_[i].transform.rotation_.x = std::atan2(-toPlayer.y, vXZ);

		projectiles_[i].transform.UpdateMatrix();
		if (vp_) projectiles_[i].model->Update(projectiles_[i].transform, *vp_);
	}
}

void EnemyAttackRangedSpecial::UpdateViewProjection(const ViewProjection& vp)
{
	vp_ = &vp;
	for (auto& spike : projectiles_) {
		spike.transform.UpdateMatrix();
		spike.model->Update(spike.transform, vp);
	}
}

void EnemyAttackRangedSpecial::Draw(const ViewProjection& viewProjection)
{
	for (auto& spike : projectiles_) {
		spike.model->Draw(spike.transform, viewProjection);
	}
}

void EnemyAttackRangedSpecial::Interrupt()
{
	phase_ = Phase::kNone;
	isComplete_ = true;
	projectiles_.clear();
}

void EnemyAttackRangedSpecial::ApplyVariables()
{
	kPrepTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Prep Time"));
	kRecoveryTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Recovery Time"));
	kLaunchDuration_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Launch Duration"));
	kSpikeSpeed_ = variables_->GetFloatValue(kGroupName_, "Spike Speed");
	kHomingStrength_ = variables_->GetFloatValue(kGroupName_, "Homing Strength");
	kSpikeLifeTime_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Spike Life Time"));
	kSpikeCount_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Spike Count"));
	kDamage_ = variables_->GetIntValue(kGroupName_, "Damage");
}
