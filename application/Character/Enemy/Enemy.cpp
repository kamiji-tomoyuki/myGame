#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"
#include "PlayerArm.h"

#include "EnemyAttackManager.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"

// State
#include "EnemyStatePlaying.h"
#include "EnemyStateGameOver.h"
#include "EnemyStateGameClear.h"

#include "myMath.h"

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

	// 現在の状態を更新し、次状態が返ってきたら遷移する
	if (currentState_) {
		std::unique_ptr<IEnemyState> next = currentState_->Update(this);
		if (next) {
			ChangeState(std::move(next));
		}
	}

	BaseObject::Update();
	obj3d_->Update(BaseObject::GetWorldTransform(), vp);

	// HPバー更新
	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	float currentWidth = kHpBarFullWidth_ * hpRatio;
	hpBar_->SetSize({ currentWidth, kHpBarHeight_ });

	// 遠距離攻撃のビュープロジェクション更新
	if (vp_ != nullptr && attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->UpdateViewProjection(vp);
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
	if (!isAlive_) { return; }

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
	float   distance = direction.Length();

	float totalRadius = GetRadius() + player->GetRadius();
	if (distance < totalRadius && distance > 0.0f) {
		direction = direction.Normalize();
		float overlap = totalRadius - distance;

		if (attackManager_ && attackManager_->IsAttacking()) {
			overlap *= kAttackOverlapMultiplier_;
		}

		Vector3 pushVector = direction * overlap;
		player->SetTranslation(playerPos - pushVector);

		Vector3 playerVelocity = player->GetVelocity();
		float   dotProduct = playerVelocity.Dot(-direction);
		if (dotProduct > 0) {
			Vector3 projectedVelocity = -direction * dotProduct;
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
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);

	if (attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->Draw(viewProjection);
		}
	}
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{
	// アニメーション描画（拡張予定）
}

void Enemy::DrawParticle(const ViewProjection& viewProjection)
{
	if (attackManager_) {
		if (auto* meleeAttack = attackManager_->GetMeleeAttack()) {
			meleeAttack->DrawTrailEffect();
		}
	}
}

void Enemy::DrawSprite(const ViewProjection& viewProjection)
{
	hpBarBg_->Draw();
	hpBar_->Draw();
}

void Enemy::ImGui()
{
	// ImGui処理
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