#include "Player.h"
#include "Input.h"
#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"
#include "myMath.h"
#include <Enemy.h>
#include <EnemyAttackManager.h>

Player::Player()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void Player::Init()
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	stageManager_ = StageManager::GetInstance();
	stageManager_->Initialize();

	BaseObject::SetWorldPosition({ 0.0f, 0.0f, -15.0f });
	BaseObject::SetScale({ 0.0f, 0.0f, 0.0f });

	// モデル
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Player/playerBody.obj");

	// HPバー背景
	hpBarBg_ = std::make_unique<Sprite>();
	hpBarBg_->Initialize("white1x1.png", { 40.0f - kHpBarBgPadding_, 150.0f - kHpBarBgPadding_ });
	hpBarBg_->SetColor({ 0.2f, 0.2f, 0.2f });
	hpBarBg_->SetSize({ kHpBarFullWidth_ + kHpBarBgPadding_ * 2.0f, kHpBarHeight_ + kHpBarBgPadding_ * 2.0f });

	// HPバー
	hpBar_ = std::make_unique<Sprite>();
	hpBar_->Initialize("white1x1.png", { 40.0f, 150.0f });
	hpBar_->SetColor(hpColor_);
	hpBar_->SetSize({ kHpBarFullWidth_, kHpBarHeight_ });

	InitArm();

	// エフェクト
	hitEffect_ = std::make_unique<ParticleEmitter>();
	hitEffect_->Initialize("hitEffect", "debug/ringPlane.obj");

	damageEffect_ = std::make_unique<ParticleEmitter>();
	damageEffect_->Initialize("playerDamage", "debug/ringPlane.obj");

	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("playerTrail", "debug/plane.obj");

	// =============================================================
	// サブシステムの生成・初期化
	// =============================================================
	hitReaction_ = std::make_unique<PlayerHitReaction>();
	hitReaction_->Init();
	hitReaction_->SetDamageEmitter(damageEffect_.get());

	move_ = std::make_unique<PlayerMove>();
	move_->Init(stageManager_);
	move_->SetTrailEmitter(trailEffect_.get());
	// 軌跡パーティクルの初期位置を設定（元コードと同じ）
	move_->SetLastTrailPosition(BaseObject::GetWorldPosition());

	rushPosture_ = std::make_unique<PlayerRushPosture>();
	rushPosture_->Init(followCamera_, stageManager_);

	// 既存サブクラス
	startEffect_ = std::make_unique<PlayerStartEffect>(this);
	gameOverEffect_ = std::make_unique<PlayerGameOverEffect>(this);
	gameClearEffect_ = std::make_unique<PlayerGameClearEffect>(this);
	attack_ = std::make_unique<PlayerAttack>(this, arms_);
	dodge_ = std::make_unique<PlayerDodge>(this, stageManager_);

	isAlive_ = true;
	gameState_ = GameState::kPlaying;
}

// =============================================================
//  開始演出
// =============================================================
void Player::UpdateStartEffect()
{
	BaseObject::Update();
	startEffect_->Update();
}

// =============================================================
//  メイン Update
// =============================================================
void Player::Update()
{
	BaseObject::Update();

	switch (gameState_) {
	case GameState::kPlaying:
		if (isAlive_) {
			hitReaction_->UpdateContactCooldown();

			if (hitReaction_->IsHitReacting()) {
				// Update でシェイクオフセットを計算させるだけ（実座標は動かさない）
				hitReaction_->Update();
			}
			else if (dodge_->IsDodging()) {
				behavior_ = Behavior::kDodge;
				dodge_->Update();
			}
			else {
				behavior_ = Behavior::kRoot;
				hitEffect_->SetPosition(BaseObject::GetWorldPosition());
				Move();
				attack_->Update();
				UpdateLockOn();
			}
		}

		obj3d_->UpdateAnimation(true);
		UpdateArms();

		// ラッシュ姿勢
		{
			Vector3 currentRot = BaseObject::GetTransform().rotation_;
			Vector3 newRot = currentRot;
			rushPosture_->UpdateBodyPosture(arms_, isLockOn_, currentRot, newRot);

			// ひねり量を抑制（元の回転との差分をスケールダウン）
			const float kTwistScale = 0.4f;
			newRot.y = currentRot.y + (newRot.y - currentRot.y) * kTwistScale;
			newRot.z = currentRot.z + (newRot.z - currentRot.z) * kTwistScale;

			BaseObject::SetRotation(newRot);
		}

		// フィニッシャー前進（ラッシュ中のみ座標を上書き）
		{
			Vector3 currentPos = BaseObject::GetWorldPosition();
			Vector3 newPos = currentPos;
			rushPosture_->UpdateFinisherAdvance(
				arms_,
				currentPos,
				BaseObject::GetTransform().rotation_.y,
				newPos);
			if (rushPosture_->IsFinisherAdvancing()) {
				BaseObject::SetWorldPosition(newPos);
			}
		}

		break;

	case GameState::kGameOver:
		gameOverEffect_->Update();
		isAlive_ = gameOverEffect_->IsAlive();
		break;

	case GameState::kGameClear:
		gameClearEffect_->Update();
		break;
	}

	// HPバー更新
	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	float currentWidth = kHpBarFullWidth_ * hpRatio;
	hpBar_->SetSize({ currentWidth, kHpBarHeight_ });

	trailEffect_->UpdateOnce(*vp_);
	Collider::UpdateWorldTransform();

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
	damageEffect_->Update(*vp_);
#endif
}

// =============================================================
//  移動（PlayerMoveに委譲）
// =============================================================
void Player::Move()
{
	Vector3 currentPos = BaseObject::GetWorldPosition();
	float   currentRotY = BaseObject::GetTransform().rotation_.y;
	Vector3 currentRot = BaseObject::GetTransform().rotation_;

	Vector3 newPos = currentPos;
	float   newRotY = currentRotY;

	Vector3 dodgeDir = move_->Update(currentPos, currentRotY, newPos, newRotY);

	// Y軸回転を反映
	currentRot.y = newRotY;
	BaseObject::SetRotation(currentRot);

	// 回避リクエストがあれば Dodge に渡す
	if (move_->IsDodgeRequested()) {
		dodge_->Start(dodgeDir);
		behavior_ = Behavior::kDodge;
		return;
	}

	BaseObject::SetWorldPosition(newPos);
}

// =============================================================
//  被弾（PlayerHitReactionに委譲）
// =============================================================
void Player::TakeDamage(const Vector3& hitPosition)
{
	if (hitReaction_->IsHitReacting() || dodge_->IsDodging()) { return; }
	hitReaction_->Start(BaseObject::GetWorldPosition(), hitPosition);
}

// =============================================================
//  外部ダメージ
// =============================================================
void Player::ApplyDamage(uint32_t damage, const Vector3& hitPosition)
{
	if (dodge_->IsDodging() || hitReaction_->IsHitReacting()) { return; }

	if (HP_ > damage) { HP_ -= damage; }
	else {
		HP_ = 0;
		gameState_ = GameState::kGameOver;
	}

	TakeDamage(hitPosition);
}

// =============================================================
//  ロックオン
// =============================================================
void Player::UpdateLockOn()
{
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		isLockOn_ = !isLockOn_;
	}

	if (isLockOn_ && enemy_ != nullptr) {
		Vector3 playerPos = GetCenterPosition();
		Vector3 enemyPos = enemy_->GetCenterPosition();
		Vector3 dir = (enemyPos - playerPos).Normalize();

		float targetRotY = std::atan2(dir.x, dir.z);
		float currentRotY = GetCenterRotation().y;

		float diff = targetRotY - currentRotY;
		const float PI = 3.14159265359f;
		while (diff > PI) { diff -= 2.0f * PI; }
		while (diff < -PI) { diff += 2.0f * PI; }

		float newRotY = currentRotY + diff * 0.2f;
		Vector3 rot = GetCenterRotation();
		BaseObject::SetRotation({ rot.x, newRotY, rot.z });

		lockOnAngleY_ = targetRotY;
		if (followCamera_) { followCamera_->SetStableAngleY(lockOnAngleY_); }
	}
	else {
		if (followCamera_) { followCamera_->ClearStableAngle(); }
	}
}

// =============================================================
//  腕・アニメーション更新
// =============================================================
void Player::UpdateArms()
{
	for (const auto& arm : arms_) {
		if (arm) { arm->Update(); }
	}
}

void Player::UpdateModelAnimation()
{
	obj3d_->UpdateAnimation(true);
}

// =============================================================
//  UI判定・攻撃実行中判定（PlayerAttackに委譲）
// =============================================================
bool Player::CanRightPunch()      const { return attack_->CanRightPunch(); }
bool Player::CanLeftPunch()       const { return attack_->CanLeftPunch(); }
bool Player::CanRush()            const { return attack_->CanRush(); }
bool Player::IsRightPunchActive() const { return attack_->IsRightPunchActive(); }
bool Player::IsLeftPunchActive()  const { return attack_->IsLeftPunchActive(); }
bool Player::IsRushActive()       const { return attack_->IsRushActive(); }

// =============================================================
//  velocity getter/setter（PlayerMoveに委譲）
// =============================================================
Vector3 Player::GetVelocity() const { return move_ ? move_->GetVelocity() : Vector3{}; }
void    Player::SetVelocity(const Vector3& v) { if (move_) { move_->SetVelocity(v); } }

// =============================================================
//  描画
// =============================================================
void Player::Draw(const ViewProjection& viewProjection)
{
	if (!isAlive_) { return; }

	WorldTransform tempTransform = BaseObject::GetWorldTransform();
	if (hitReaction_->IsHitReacting()) {
		// 実座標はそのままに、描画位置だけオフセットを乗せる（元コードと同じ方式）
		tempTransform.translation_ += hitReaction_->GetShakeOffset();
		tempTransform.UpdateMatrix();
	}
	obj3d_->Draw(tempTransform, viewProjection);
}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	if (!isAlive_) { return; }
	for (const auto& arm : arms_) {
		if (arm) { arm->DrawAnimation(viewProjection); }
	}
}

void Player::DrawSprite(const ViewProjection& viewProjection)
{
	hpBarBg_->Draw();
	hpBar_->Draw();
}

void Player::DrawParticle(const ViewProjection& viewProjection)
{
	hitEffect_->Draw(Ring);
	damageEffect_->Draw(Ring);
	trailEffect_->Draw(Normal);
}

// =============================================================
//  ImGui
// =============================================================
void Player::ImGui()
{
	hitEffect_->imgui();
	damageEffect_->imgui();
	trailEffect_->imgui();
}

// =============================================================
//  OnCollision
// =============================================================
void Player::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		if (dodge_->IsDodging() || hitReaction_->IsHitReacting()) { return; }

		if (enemy->IsAttacking()) {
			uint32_t dmg = 100;
			if (HP_ > dmg) { HP_ -= dmg; }
			else { HP_ = 0; gameState_ = GameState::kGameOver; }

			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			TakeDamage(hitPos);

			Vector3 knockback = (GetCenterPosition() - enemy->GetCenterPosition()).Normalize();
			move_->SetVelocity(move_->GetVelocity() + knockback * 0.5f);
		}
		else {
			if (hitReaction_->IsContactCooldownActive()) { return; }

			uint32_t dmg = 10;
			if (HP_ > dmg) { HP_ -= dmg; }
			else { HP_ = 0; gameState_ = GameState::kGameOver; }

			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			TakeDamage(hitPos);
		}

		Vector3 playerPos = BaseObject::GetWorldPosition();
		if (stageManager_ && !stageManager_->IsWithinStageBounds(playerPos)) {
			BaseObject::SetWorldPosition(stageManager_->ClampToStageBounds(playerPos));
		}
	}

	transform_.UpdateMatrix();
}

// =============================================================
//  InitArm
// =============================================================
void Player::InitArm()
{
	arms_[kRArm] = std::make_unique<PlayerArm>();
	arms_[kRArm]->Init("player/Arm/playerArm.gltf");
	arms_[kRArm]->SetPlayer(this);
	arms_[kRArm]->SetID(serialNumber_);
	arms_[kRArm]->SetColliderID(CollisionTypeIdDef::kPRArm);
	arms_[kRArm]->SetIsRightArm(true);
	arms_[kRArm]->SetTranslation(Vector3(1.7f, 0.0f, 1.3f));
	arms_[kRArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));

	arms_[kLArm] = std::make_unique<PlayerArm>();
	arms_[kLArm]->Init("player/Arm/playerArm.gltf");
	arms_[kLArm]->SetPlayer(this);
	arms_[kLArm]->SetID(serialNumber_);
	arms_[kLArm]->SetColliderID(CollisionTypeIdDef::kPLArm);
	arms_[kLArm]->SetIsRightArm(false);
	arms_[kLArm]->SetTranslation(Vector3(-1.7f, 0.0f, 1.3f));
	arms_[kLArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));
}