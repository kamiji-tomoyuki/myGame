#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"
#include "PlayerArm.h"
#include <ParticleEmitter.h>

#include "EnemyAttackManager.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"
#include "EnemyAttackRangedSpecial.h"
#include "EnemyAttackCircle.h"

// State
#include "EnemyStatePlaying.h"
#include "EnemyStateGameOver.h"
#include "EnemyStateGameClear.h"

#include "myMath.h"

using namespace Engine;
uint32_t Enemy::nextSerialNumber_ = 0;
const std::string Enemy::kGroupName_ = "Enemy";

Enemy::Enemy()
{
	serialNumber_ = nextSerialNumber_;
	++nextSerialNumber_;
}

void Enemy::Init()
{
	BaseObject::Init();
	BaseObject::SetWorldPosition(kInitialWorldPosition_);

	// =============================================================
	// GlobalVariables の初期化
	// =============================================================
	variables_ = GlobalVariables::GetInstance();
	if (!variables_->GroupExists(kGroupName_)) {
		variables_->CreateGroup(kGroupName_);
	}
	variables_->AddItem(kGroupName_, "Max HP", static_cast<int32_t>(kMaxHP_Adjustable_));
	ApplyVariables();

	float size = kColliderSize_;

	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	Collider::SetRadius(size);

	// --- モデル ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Enemy/enemyBody.obj");
	obj3d_->SetSize({ size, size, size });
	obj3d_->SetRotation(kModelInitialRotation_);
	BaseObject::SetRotation(obj3d_->GetRotation());
	BaseObject::SetScale(obj3d_->GetSize());

	originalRotation_ = obj3d_->GetRotation();

	// --- 攻撃管理 ---
	attackManager_ = std::make_unique<EnemyAttackManager>();
	attackManager_->Initialize();

	// --- コンポーネント ---
	move_ = std::make_unique<EnemyMove>();
	hitReaction_ = std::make_unique<EnemyHitReaction>();

	// --- エフェクト ---
	effect_ = std::make_unique<EnemyEffect>();

	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("enemyTrail", "debug/ringPlane.obj");
	trailEffect_->SetActive(false);

	powerUpEffect_ = std::make_unique<ParticleEmitter>();
	powerUpEffect_->Initialize("EnemyPowerUp", "debug/ringPlane.obj");
	powerUpEffect_->SetActive(false);

	// --- コンポーネントへのエフェクト設定 ---
	move_->SetTrailEmitter(trailEffect_.get());
	move_->SetLastTrailPosition(BaseObject::GetWorldPosition());

	// --- HPバー ---
	hpBarBg_ = std::make_unique<Sprite>();
	hpBarBg_->Initialize("white1x1.png", {
		kHpBarPosX_ + kHpBarBgPadding_,
		kHpBarPosY_ - kHpBarBgPadding_ });
	hpBarBg_->SetAnchorPoint({ 1.0f, 0.0f });
	hpBarBg_->SetColor({ kHpBarBgColor_.x, kHpBarBgColor_.y, kHpBarBgColor_.z });
	hpBarBg_->SetSize({
		kHpBarFullWidth_ + kHpBarBgPadding_ * 2.0f,
		kHpBarHeight_ + kHpBarBgPadding_ * 2.0f });

	hpBar_ = std::make_unique<Sprite>();
	hpBar_->Initialize("white1x1.png", { kHpBarPosX_, kHpBarPosY_ });
	hpBar_->SetAnchorPoint({ 1.0f, 0.0f });
	hpBar_->SetColor({ 1.0f, 0.0f, 0.0f });
	hpBar_->SetSize({ kHpBarFullWidth_, kHpBarHeight_ });

	// --- State Pattern：初期状態を Playing に設定 ---
	ChangeState(std::make_unique<EnemyStatePlaying>());
}

// =============================================================
//  State の切り替え（Enter / Exit を確実に呼ぶ）
// =============================================================
void Enemy::ChangeState(std::unique_ptr<IEnemyState> next)
{
	if (currentState_) {
		currentState_->Exit(this);
	}
	currentState_ = std::move(next);
	if (currentState_) {
		currentState_->Enter(this);
	}
}

// =============================================================
//  メイン Update — currentState_ へ委譲
// =============================================================
void Enemy::Update(Player* player, const ViewProjection& vp)
{
	player_ = player;
	vp_ = &vp;
	ApplyVariables();

	// デバッグ一時停止中は状態・移動・攻撃の更新をスキップ
	if (!isPaused_)
	{
		hitReaction_->UpdateWobble();

		// 現在の状態を更新し、次状態が返ってきたら遷移する
		if (currentState_) {
			std::unique_ptr<IEnemyState> next = currentState_->Update(this);
			if (next) {
				ChangeState(std::move(next));
			}
		}
	}

	BaseObject::Update();
	obj3d_->Update(BaseObject::GetWorldTransform(), vp);

	trailEffect_->Update(vp);
	powerUpEffect_->Update(vp);

	// HPバー更新
	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	float currentWidth = kHpBarFullWidth_ * hpRatio;
	hpBar_->SetSize({ currentWidth, kHpBarHeight_ });

	// 遠距離攻撃のビュープロジェクション更新
	if (vp_ != nullptr && attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->UpdateViewProjection(vp);
		}
		if (auto* rangedAttackSpecial = attackManager_->GetRangedAttackSpecial()) {
			rangedAttackSpecial->UpdateViewProjection(vp);
		}
		if (auto* circleAttack = attackManager_->GetCircleAttack()) {
			circleAttack->UpdateViewProjection(vp);
		}
	}
}

// =============================================================
//  開始演出（EnemyEffect へ委譲）
// =============================================================
void Enemy::UpdateStartEffect()
{
	effect_->UpdateStartEffect(this);
}

// =============================================================
//  被弾（EnemyHitReaction へ委譲）
// =============================================================
void Enemy::OnRushHit(bool isFinalHit)
{
	hitReaction_->OnRushHit(isFinalHit, this);
}

// =============================================================
//  ダメージ
// =============================================================
void Enemy::TakeDamage(uint32_t damage)
{
	if (!isAlive_ || isInvincible_) { return; }

	hitReaction_->OnHit();

	if (HP_ > damage) {
		HP_ -= damage;
	}
	else {
		HP_ = 0;
		isAlive_ = false;
	}
}

bool Enemy::IsAttacking() const
{
	return attackManager_ ? attackManager_->IsAttacking() : false;
}

// =============================================================
//  衝突処理
// =============================================================
void Enemy::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kPlayer)) {
		Player* player = static_cast<Player*>(other);

		if (HP_ == 0) {
			isAlive_ = false;
		}

		// 重複処理を避けるためシリアル番号の小さい方で処理
		if (GetSerialNumber() > player->GetSerialNumber()) {
			return;
		}

		HandleCollisionWithPlayer(player);
	}
}

void Enemy::HandleCollisionWithPlayer(Player* player)
{
	Vector3 enemyPos = GetCenterPosition();
	Vector3 playerPos = player->GetCenterPosition();
	Vector3 direction = enemyPos - playerPos;
	
	// 水平方向の距離と方向を計算
	Vector3 directionXZ = { direction.x, 0.0f, direction.z };
	float   distanceXZ = directionXZ.Length();

	float totalRadius = GetRadius() + player->GetRadius();
	
	// Y軸の重なりも考慮しつつ、押し出しは水平のみにする
	float distanceY = std::abs(direction.y);
	float heightSum = GetRadius() + player->GetRadius(); // 簡易的に半径を高さとして使用

	if (distanceXZ < totalRadius && distanceXZ > 0.0f && distanceY < heightSum) {
		directionXZ = directionXZ.Normalize();
		float overlap = totalRadius - distanceXZ;

		if (attackManager_ && attackManager_->IsAttacking()) {
			overlap *= kAttackOverlapMultiplier_;
		}

		Vector3 pushVector = directionXZ * overlap;
		// プレイヤーを水平方向に押し戻す
		player->SetTranslation(playerPos - pushVector);

		Vector3 playerVelocity = player->GetVelocity();
		float   dotProduct = playerVelocity.Dot(-directionXZ);
		if (dotProduct > 0) {
			Vector3 projectedVelocity = -directionXZ * dotProduct;
			playerVelocity -= projectedVelocity;
			player->SetVelocity(playerVelocity);
		}

		if (!attackManager_ || !attackManager_->IsAttacking()) {
			if (move_) {
				move_->SetVelocity(move_->GetVelocity() * kCollisionVelocityDamping_);
			}
		}

		transform_.UpdateMatrix();
	}
}

// =============================================================
//  描画
// =============================================================
void Enemy::Draw(const ViewProjection& viewProjection)
{
	if (hitReaction_->IsWobbling()) {
		// 行列計算ではなく、SRTを一時的に変更して Object3d::Draw に渡す
		// Object3d::Update が内部で SRT から行列を再計算するため
		Vector3 originalPos = transform_.translation_;
		Vector3 originalRot = transform_.rotation_;

		Vector3 wobbleRot = hitReaction_->GetWobbleRotation();
		float halfHeight = kColliderSize_ * 0.5f;
		
		// 足元を支点とした回転後の座標を計算
		Vector3 footPos = originalPos;
		footPos.y -= halfHeight;

		Matrix4x4 matRotate = MakeRotateXYZMatrix(wobbleRot);
		Vector3 localOffset = { 0.0f, halfHeight, 0.0f };
		Vector3 rotatedOffset = Transformation(localOffset, matRotate);

		transform_.translation_ = footPos + rotatedOffset;
		transform_.rotation_ = originalRot + wobbleRot;
		transform_.UpdateMatrix();

		obj3d_->Draw(transform_, viewProjection);

		// 元に戻す
		transform_.translation_ = originalPos;
		transform_.rotation_ = originalRot;
		transform_.UpdateMatrix();
	}
	else {
		obj3d_->Draw(transform_, viewProjection);
	}

	if (attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->Draw(viewProjection);
		}
		if (auto* rangedAttackSpecial = attackManager_->GetRangedAttackSpecial()) {
			rangedAttackSpecial->Draw(viewProjection);
		}
		if (auto* circleAttack = attackManager_->GetCircleAttack()) {
			circleAttack->Draw(viewProjection);
		}
	}
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{
	// アニメーション描画（拡張予定）
}

void Enemy::DrawParticle(const ViewProjection& viewProjection)
{
	trailEffect_->Draw(Ring);
	powerUpEffect_->Draw(Ring);
}

void Enemy::DrawSprite(const ViewProjection& viewProjection)
{
	hpBarBg_->Draw();
	hpBar_->Draw();
}

void Enemy::ImGui()
{
#ifdef _DEBUG
	ImGui::Begin("Enemy Debug");

	// ===== 一時停止 / 再開 ボタン =====
	if (isPaused_) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
		if (ImGui::Button("Resume  (F5)", ImVec2(160, 40))) {
			isPaused_ = false;
		}
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
		if (ImGui::Button("Pause   (F5)", ImVec2(160, 40))) {
			isPaused_ = true;
		}
	}
	ImGui::PopStyleColor(3);

	// キーボードショートカット（F5 で切り替え）
	if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
		isPaused_ = !isPaused_;
	}

	ImGui::SameLine();
	ImGui::TextColored(
		isPaused_ ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
		isPaused_ ? "[ PAUSED ]" : "[ RUNNING ]"
	);

	ImGui::Separator();

	// ===== ステータス表示 =====
	ImGui::Text("HP        : %u / %u", HP_, kMaxHP_);
	ImGui::Text("IsAlive   : %s", isAlive_ ? "true" : "false");
	ImGui::Text("IsAttacking: %s", IsAttacking() ? "true" : "false");

	if (move_) {
		Vector3 vel = move_->GetVelocity();
		ImGui::Text("Velocity  : (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
	}

	Vector3 pos = GetCenterPosition();
	ImGui::Text("Position  : (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

	ImGui::Separator();

	// ===== 攻撃デバッグ =====
	if (ImGui::CollapsingHeader("Attack Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (attackManager_) {
			if (ImGui::Button("FORCE: Melee Attack", ImVec2(200, 30))) {
				attackManager_->DebugTriggerAttack(EnemyAttackManager::AttackType::kMelee, this, player_);
			}
			if (ImGui::Button("FORCE: Ranged Attack", ImVec2(200, 30))) {
				attackManager_->DebugTriggerAttack(EnemyAttackManager::AttackType::kRanged, this, player_);
			}
			if (ImGui::Button("FORCE: Special Ranged", ImVec2(200, 30))) {
				attackManager_->DebugTriggerAttack(EnemyAttackManager::AttackType::kRangedSpecial, this, player_);
			}
			if (ImGui::Button("FORCE: Circle Attack", ImVec2(200, 30))) {
				attackManager_->DebugTriggerAttack(EnemyAttackManager::AttackType::kCircle, this, player_);
			}
			if (ImGui::Button("Interrupt Attack", ImVec2(200, 30))) {
				attackManager_->InterruptByRush(this);
			}
		}
	}

	ImGui::Separator();

	// ===== 個別停止オプション（一時停止中のみ有効）=====
	ImGui::BeginDisabled(!isPaused_);
	ImGui::Text("-- Pause Options (enabled while paused) --");

	static bool freezeAttack = true;
	static bool freezeMove = true;
	ImGui::Checkbox("Freeze Attack", &freezeAttack);
	ImGui::Checkbox("Freeze Move", &freezeMove);
	ImGui::EndDisabled();

	ImGui::End();
	// パーティクル(trailEffect_)は集約「パーティクル」窓で編集する
#endif // _DEBUG
}

// =============================================================
//  ApplyVariables — GlobalVariables から値を取得して反映
// =============================================================
void Enemy::ApplyVariables()
{
	kMaxHP_Adjustable_ = static_cast<uint32_t>(variables_->GetIntValue(kGroupName_, "Max HP"));
	kMaxHP_ = kMaxHP_Adjustable_;
}

// =============================================================
//  obj3d_ アクセサ（コンポーネント・状態クラスから使用）
// =============================================================
Vector3 Enemy::GetObjRotation() const
{
	return obj3d_ ? obj3d_->GetRotation() : Vector3{};
}

void Enemy::SetObjRotation(const Vector3& rot)
{
	if (obj3d_) { obj3d_->SetRotation(rot); }
}