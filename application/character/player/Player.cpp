#include "Player.h"
#include "Input.h"
#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"
#include "myMath.h"
#include <Enemy.h>
#include <EnemyAttackManager.h>

// State Pattern
#include "PlayerStatePlaying.h"
#include "PlayerStateGameOver.h"
#include "PlayerStateGameClear.h"

#include <numbers>

const std::string Player::kGroupName_ = "Player";

Player::Player()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void Player::Init()
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// =============================================================
	// GlobalVariables の初期化
	// =============================================================
	variables_ = GlobalVariables::GetInstance();
	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}
	variables_->AddItem(kGroupName_, "Max HP", static_cast<int32_t>(kMaxHP_Adjustable_));
	variables_->AddItem(kGroupName_, "Right Arm Translation", kRightArmTranslation_);
	variables_->AddItem(kGroupName_, "Left Arm Translation", kLeftArmTranslation_);
	variables_->AddItem(kGroupName_, "Arm Scale", kArmScale_);
	ApplyVariables();

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

	// エフェクト
	hitEffect_ = std::make_unique<ParticleEmitter>();
	hitEffect_->Initialize("hitEffect", "debug/ringPlane.obj");

	damageEffect_ = std::make_unique<ParticleEmitter>();
	damageEffect_->Initialize("playerDamage", "debug/ringPlane.obj");

	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("playerTrail", "debug/ringPlane.obj");

	// =============================================================
	// サブシステムの生成・初期化
	// =============================================================
	hitReaction_ = std::make_unique<PlayerHitReaction>();
	hitReaction_->Init();
	hitReaction_->SetDamageEmitter(damageEffect_.get());

	move_ = std::make_unique<PlayerMove>();
	move_->Init(stageManager_);
	move_->SetTrailEmitter(trailEffect_.get());
	move_->SetLastTrailPosition(BaseObject::GetWorldPosition());

	rushPosture_ = std::make_unique<PlayerRushPosture>();
	rushPosture_->Init(followCamera_, stageManager_);

	startEffect_ = std::make_unique<PlayerStartEffect>(this);
	gameOverEffect_ = std::make_unique<PlayerGameOverEffect>(this);
	gameClearEffect_ = std::make_unique<PlayerGameClearEffect>(this);
	attack_ = std::make_unique<PlayerAttack>(this, arms_);
	attack_->Init();   // ゲージの GlobalVariables 登録
	InitArm();         // attack_ 生成後に呼ぶ（SetPlayerAttack のため）
	dodge_ = std::make_unique<PlayerDodge>(this, stageManager_);

	isAlive_ = true;
	gameState_ = GameState::kPlaying;

	// =============================================================
	// State Pattern — 初期状態を Playing に設定
	// =============================================================
	ChangeState(new PlayerStatePlaying());
}

// =============================================================
//  State の切り替え（Enter / Exit を確実に呼ぶ）
// =============================================================
void Player::ChangeState(IPlayerState* next)
{
	if (currentState_) {
		currentState_->Exit(this);
	}
	currentState_.reset(next);
	if (currentState_) {
		currentState_->Enter(this);
	}
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
//  ★ 必殺技更新を State::Update より先に行うことで、
//    同フレーム内で IsUltimateActive() が正しく参照される
// =============================================================
void Player::Update()
{
	BaseObject::Update();
	ApplyVariables();

	// ★ 必殺技モーション更新を先に行う
	//    → 同フレームで State（PlayerStatePlaying）が IsUltimateActive() を
	//      正しく読めるようになる
	if (attack_) { attack_->UpdateUltimate(this, enemy_); }

	// 現在の状態を更新し、次状態が返ってきたら遷移する
	if (currentState_) {
		IPlayerState* next = currentState_->Update(this);
		if (next) {
			ChangeState(next);
		}
	}

	// HPバー更新
	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	hpBar_->SetSize({ kHpBarFullWidth_ * hpRatio, kHpBarHeight_ });

	trailEffect_->UpdateOnce(*vp_);
	Collider::UpdateWorldTransform();

	// 各クールダウン更新
	if (hitReaction_) {
		hitReaction_->UpdateContactCooldown();
		hitReaction_->UpdateRangedCooldown();
	}

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
	damageEffect_->Update(*vp_);
#endif
}

// =============================================================
//  移動（PlayerMove に委譲 — BehaviorRoot から呼ばれる）
// =============================================================
void Player::MoveInternal()
{
	// 必殺技モーション中は移動処理をスキップ（PlayerUltimate が座標を直接制御する）
	if (IsUltimateActive()) { return; }

	// ※ IsHitReacting() 中でも移動入力は受け付ける
	//   シェイク演出は Draw() 側のオフセットのみで表現するため、
	//   ここで移動をロックする必要はない

	Vector3 currentPos = BaseObject::GetWorldPosition();
	float   currentRotY = BaseObject::GetTransform().rotation_.y;
	Vector3 currentRot = BaseObject::GetTransform().rotation_;

	Vector3 newPos = currentPos;
	float   newRotY = currentRotY;

	Vector3 dodgeDir = move_->Update(currentPos, currentRotY, newPos, newRotY);

	currentRot.y = newRotY;
	BaseObject::SetRotation(currentRot);

	if (move_->IsDodgeRequested()) {
		dodge_->Start(dodgeDir);
		return;
	}

	BaseObject::SetWorldPosition(newPos);
}

// =============================================================
//  被弾（PlayerHitReaction に委譲）
// =============================================================
void Player::TakeDamage(const Vector3& hitPosition)
{
	if (hitReaction_->IsHitReacting() || dodge_->IsDodging() || IsUltimateActive()) { return; }
	hitReaction_->Start(BaseObject::GetWorldPosition(), hitPosition);
}

// =============================================================
//  外部ダメージ
// =============================================================
void Player::ApplyDamage(uint32_t damage, const Vector3& hitPosition)
{
	if (dodge_->IsDodging() || hitReaction_->IsHitReacting() || IsUltimateActive()) { return; }

	if (HP_ > damage) {
		HP_ -= damage;
	}
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
	if (enemy_ == nullptr) { return; }
	Vector3 playerPos = GetCenterPosition();
	Vector3 enemyPos = enemy_->GetCenterPosition();
	Vector3 dir = (enemyPos - playerPos).Normalize();
	const float PI = std::numbers::pi_v<float>;

	auto NormalizedDiff = [&](float target, float current) -> float {
		float diff = target - current;
		while (diff > PI) { diff -= 2.0f * PI; }
		while (diff < -PI) { diff += 2.0f * PI; }
		return diff;
		};

	float targetRotY = std::atan2(dir.x, dir.z);
	float targetRotX = -std::atan2(dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));

	Vector3 rot = GetCenterRotation();
	float newRotX = rot.x + NormalizedDiff(targetRotX, rot.x) * 0.2f;
	float newRotY = rot.y + NormalizedDiff(targetRotY, rot.y) * 0.2f;

	BaseObject::SetRotation({ newRotX, newRotY, rot.z });
	if (followCamera_) { followCamera_->SetStableAngleY(targetRotY); }
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
//  UI判定・攻撃実行中判定（PlayerAttack に委譲）
// =============================================================
bool Player::CanRightPunch()      const { return attack_->CanRightPunch(); }
bool Player::CanLeftPunch()       const { return attack_->CanLeftPunch(); }
bool Player::CanRush()            const { return attack_->CanRush(); }
bool Player::IsRightPunchActive() const { return attack_->IsRightPunchActive(); }
bool Player::IsLeftPunchActive()  const { return attack_->IsLeftPunchActive(); }
bool Player::IsRushActive()       const { return attack_->IsRushActive(); }
bool Player::IsUltimateActive()   const { return attack_ && attack_->IsUltimateActive(); }

// =============================================================
//  velocity getter/setter（PlayerMove に委譲）
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
	trailEffect_->Draw(Ring);
}

// =============================================================
//  ImGui
// =============================================================
void Player::ImGui()
{
	hitEffect_->imgui();
	damageEffect_->imgui();
	trailEffect_->imgui();
	if (attack_) { attack_->ImGui(); }
}

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void Player::ApplyVariables()
{
	kMaxHP_Adjustable_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Max HP"));
	kRightArmTranslation_ = variables_->GetVector3Value(kGroupName_, "Right Arm Translation");
	kLeftArmTranslation_ = variables_->GetVector3Value(kGroupName_, "Left Arm Translation");
	kArmScale_ = variables_->GetVector3Value(kGroupName_, "Arm Scale");

	kMaxHP_ = kMaxHP_Adjustable_;

	// ★ 必殺技モーション中は腕位置を上書きしない
	//    （PlayerUltimate が毎フレーム腕の translation を直接制御するため）
	if (!IsUltimateActive()) {
		if (arms_[kRArm]) {
			arms_[kRArm]->SetTranslation(kRightArmTranslation_);
			arms_[kRArm]->SetScale(kArmScale_);
		}
		if (arms_[kLArm]) {
			arms_[kLArm]->SetTranslation(kLeftArmTranslation_);
			arms_[kLArm]->SetScale(kArmScale_);
		}
	}
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
			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			ApplyDamage(100, hitPos);

			Vector3 knockback = (GetCenterPosition() - enemy->GetCenterPosition()).Normalize();
			move_->SetVelocity(move_->GetVelocity() + knockback * 0.5f);
		}
		else {
			if (hitReaction_->IsContactCooldownActive()) { return; }

			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			ApplyDamage(10, hitPos);
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
	arms_[kRArm]->SetTranslation(kRightArmTranslation_);
	arms_[kRArm]->SetScale(kArmScale_);
	arms_[kRArm]->SetPlayerAttack(attack_.get());

	arms_[kLArm] = std::make_unique<PlayerArm>();
	arms_[kLArm]->Init("player/Arm/playerArm.gltf");
	arms_[kLArm]->SetPlayer(this);
	arms_[kLArm]->SetID(serialNumber_);
	arms_[kLArm]->SetColliderID(CollisionTypeIdDef::kPLArm);
	arms_[kLArm]->SetIsRightArm(false);
	arms_[kLArm]->SetTranslation(kLeftArmTranslation_);
	arms_[kLArm]->SetScale(kArmScale_);
	arms_[kLArm]->SetPlayerAttack(attack_.get());
}