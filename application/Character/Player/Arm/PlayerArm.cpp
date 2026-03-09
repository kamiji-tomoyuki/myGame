#include "PlayerArm.h"
#include "Input.h"
#include "myMath.h"
#include "Player.h"
#include <Enemy.h>

PlayerArm::PlayerArm()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void PlayerArm::Init(std::string filePath)
{
	BaseObject::Init();
	Collider::Initialize();

	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize(filePath);

	attack_ = std::make_unique<PlayerArmAttack>();
	rush_ = std::make_unique<PlayerArmRush>();

	// ダメージ設定
	attack_->SetAttackDamage(50);
	rush_->SetRushAttackDamage(20);
	rush_->SetFinisherAttackDamage(150);

	originalPosition_ = transform_.translation_;

	Collider::SetRadius(0.8f);
	Collider::SetCollisionEnabled(true);
}

// =============================================================
//  更新
// =============================================================
void PlayerArm::Update()
{
	BaseObject::Update();

	switch (behavior_) {
	case Behavior::kAttack:
	{
		bool finished = attack_->Update();
		// 攻撃終了 → Normal に戻す
		if (finished) {
			behavior_ = Behavior::kNormal;
		}
		// 攻撃中の腕位置を反映
		transform_.translation_ = attack_->GetCurrentTranslation();
		break;
	}
	case Behavior::kRush:
	{
		bool finished = rush_->Update();
		// ラッシュ終了 → Normal に戻す＆攻撃タイプをリセット
		if (finished) {
			behavior_ = Behavior::kNormal;
			attack_->SetComboCount(0);
			attack_->SetComboTimer(0);
			attack_->SetLastAttackType(AttackType::kNone);
		}
		// ラッシュ中の腕位置を反映
		transform_.translation_ = rush_->GetCurrentTranslation();
		break;
	}
	default:
		break;
	}

	attack_->UpdateComboTimer();

	obj3d_->UpdateAnimation(true);
	Collider::UpdateWorldTransform();
}

// =============================================================
//  攻撃開始
// =============================================================
void PlayerArm::StartAttack(AttackType attackType)
{
	if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) { return; }

	behavior_ = Behavior::kAttack;
	attack_->StartAttack(attackType, isRightArm_, transform_.translation_);
}

// =============================================================
//  ラッシュ開始
// =============================================================
void PlayerArm::StartRush()
{
	if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) { return; }

	behavior_ = Behavior::kRush;
	rush_->StartRush(isRightArm_, transform_.translation_);
}

bool PlayerArm::CanCombo() const
{
	return (attack_->CanCombo() && behavior_ == Behavior::kNormal);
}

bool PlayerArm::CanStartRush() const
{
	return (behavior_ == Behavior::kNormal && attack_->CanStartRush());
}

Vector3 PlayerArm::GetAttackDirection() const
{
	if (behavior_ == Behavior::kRush) {
		return rush_->GetAttackDirection();
	}
	return attack_->GetAttackDirection();
}

// =============================================================
//  描画
// =============================================================
void PlayerArm::Draw(const ViewProjection& viewProjection) {}

void PlayerArm::DrawAnimation(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void PlayerArm::DrawParticle(const ViewProjection& viewProjection) {}

// =============================================================
//  OnCollision（当たり判定は本体に残す）
// =============================================================
void PlayerArm::OnCollision(Collider* other)
{
	if (!attack_->GetIsAttack() && !rush_->GetIsRush()) { return; }

	uint32_t typeID = other->GetTypeID();

	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		// -------------------------------------------------------
		// フィニッシャー判定（右腕専用・最優先）
		// -------------------------------------------------------
		if (rush_->GetIsRush() && isRightArm_ &&
			rush_->GetRushPhase() == RushPhase::kFinisher && rush_->IsFinisherHitFrame()) {
			if (!rush_->HasFinisherHit()) {
				enemy->TakeDamage(rush_->GetFinisherAttackDamage());
				enemy->OnRushHit(true);
				rush_->SetHasFinisherHit(true);
				// isFinisherHitFrame_ は PlayerArmRush 側でウィンドウを抜けたら自動的にオフになる
			}
		}
		// -------------------------------------------------------
		// 連続パンチ判定
		// -------------------------------------------------------
		else if (rush_->GetIsRush() &&
			rush_->GetRushPhase() == RushPhase::kRapidPunch && rush_->IsRushAttackActive()) {
			uint32_t rushAttackTimer = rush_->GetRushAttackTimer();
			if (rushAttackTimer >= 2 && rushAttackTimer <= 6) {
				int currentFrame = static_cast<int>(rush_->GetRushTimer());
				if (currentFrame - rush_->GetLastRushHitFrame() >= 3) {
					enemy->TakeDamage(rush_->GetRushAttackDamage());
					enemy->OnRushHit(false);
					rush_->SetLastRushHitFrame(currentFrame);
				}
			}
		}
		// -------------------------------------------------------
		// 通常攻撃判定
		// -------------------------------------------------------
		else if (attack_->GetIsAttack()) {
			float progress = attack_->GetAttackProgress();
			if (progress >= 0.4f && progress <= 0.6f && !attack_->GetHasHitThisAttack()) {
				enemy->TakeDamage(attack_->GetAttackDamage());
				attack_->SetHasHitThisAttack(true);	// 1回ヒットしたらフラグを立てる
			}
		}
	}
}

// =============================================================
//  SetPlayer
// =============================================================
void PlayerArm::SetPlayer(Player* player)
{
	player_ = player;
	BaseObject::transform_.parent_ = &player->GetWorldTransform();
}