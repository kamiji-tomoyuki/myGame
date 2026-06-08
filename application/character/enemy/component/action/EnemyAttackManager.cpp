#include "EnemyAttackManager.h"
#include "Enemy.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"
#include "EnemyAttackRangedSpecial.h"
#include "Player.h"

// =============================================================
//  関数ポインタテーブルの定義
// =============================================================
const std::unordered_map<EnemyAttackManager::AttackType, EnemyAttackManager::UpdateFunc>
EnemyAttackManager::kUpdateTable_ = {
	{ AttackType::kMelee,         &EnemyAttackManager::UpdateMelee         },
	{ AttackType::kRanged,        &EnemyAttackManager::UpdateRanged        },
	{ AttackType::kRangedSpecial, &EnemyAttackManager::UpdateRangedSpecial },
};

const std::unordered_map<EnemyAttackManager::AttackType, EnemyAttackManager::CompleteFunc>
EnemyAttackManager::kCompleteTable_ = {
	{ AttackType::kMelee,         &EnemyAttackManager::IsCompleteMelee         },
	{ AttackType::kRanged,        &EnemyAttackManager::IsCompleteRanged        },
	{ AttackType::kRangedSpecial, &EnemyAttackManager::IsCompleteRangedSpecial },
};

EnemyAttackManager::EnemyAttackManager()
{
	meleeAttack_ = std::make_unique<EnemyAttackMelee>();
	rangedAttack_ = std::make_unique<EnemyAttackRanged>();
	rangedAttackSpecial_ = std::make_unique<EnemyAttackRangedSpecial>();
}

EnemyAttackManager::~EnemyAttackManager() = default;

void EnemyAttackManager::Initialize()
{
	meleeAttack_->Initialize();
	rangedAttack_->Initialize();
	rangedAttackSpecial_->Initialize();
	currentAttackType_ = AttackType::kNone;
	attackPreparationTimer_ = 0;
}

// =============================================================
//  更新
// =============================================================
void EnemyAttackManager::Update(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) { return; }

	if (currentAttackType_ == AttackType::kNone) {
		attackPreparationTimer_++;

		uint32_t effectivePrepTime = kAttackPreparationTime_;
		if (enemy->GetIsPhase2()) {
			effectivePrepTime = static_cast<uint32_t>(kAttackPreparationTime_ * 0.7f);
		}

		if (attackPreparationTimer_ >= effectivePrepTime) {
			SelectAndStartAttack(enemy, player);
		}
		return;
	}

	// テーブルから現在の AttackType に対応する更新関数を引き
	// 完了（true）が返ったら攻撃をリセットする
	auto it = kUpdateTable_.find(currentAttackType_);
	if (it != kUpdateTable_.end()) {
		bool finished = (this->*(it->second))(enemy, player);
		if (finished) {
			ResetAttack();
		}
	}
}

// =============================================================
//  AttackType 別の更新関数
// =============================================================
bool EnemyAttackManager::UpdateMelee(Enemy* enemy, Player* player)
{
	meleeAttack_->Update(enemy, player);
	return meleeAttack_->IsComplete();
}

bool EnemyAttackManager::UpdateRanged(Enemy* enemy, Player* player)
{
	rangedAttack_->Update(enemy, player);
	return rangedAttack_->IsComplete();
}

bool EnemyAttackManager::UpdateRangedSpecial(Enemy* enemy, Player* player)
{
	rangedAttackSpecial_->Update(enemy, player);
	return rangedAttackSpecial_->IsComplete();
}

// =============================================================
//  AttackType 別の完了チェック関数
// =============================================================
bool EnemyAttackManager::IsCompleteMelee() const
{
	return meleeAttack_->IsComplete();
}

bool EnemyAttackManager::IsCompleteRanged() const
{
	return rangedAttack_->IsComplete();
}

bool EnemyAttackManager::IsCompleteRangedSpecial() const
{
	return rangedAttackSpecial_->IsComplete();
}

// =============================================================
//  IsAttackComplete
// =============================================================
bool EnemyAttackManager::IsAttackComplete() const
{
	auto it = kCompleteTable_.find(currentAttackType_);
	if (it != kCompleteTable_.end()) {
		return (this->*(it->second))();
	}
	return true; // kNone
}

// =============================================================
//  SelectAndStartAttack
// =============================================================
void EnemyAttackManager::SelectAndStartAttack(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) { return; }

	// XZ平面での距離を計算
	Vector3 toPlayer = player->GetCenterPosition() - enemy->GetCenterPosition();
	toPlayer.y = 0.0f;
	float distanceToPlayerXZ = toPlayer.Length();

	bool useMelee = false;
	if (distanceToPlayerXZ <= kMeleeAttackRange_) {
		// 近距離なら近接優先
		// 通常: 70%, Phase2: 60%
		int meleeProb = enemy->GetIsPhase2() ? 60 : 70;
		useMelee = (static_cast<int>(rand() % 100) < meleeProb);
	}
	else {
		// 遠距離なら遠距離優先
		// 通常: 30%, Phase2: 15%
		int meleeProb = enemy->GetIsPhase2() ? 15 : 30;
		useMelee = (static_cast<int>(rand() % 100) < meleeProb);
	}

	if (useMelee) {
		currentAttackType_ = AttackType::kMelee;
		meleeAttack_->Start(enemy, player);
	}
	else {
		// 遠距離攻撃が選択された場合
		// 強化状態(Phase2)かつHPが50%以下のとき、70%の確率で特殊攻撃を選択
		bool useSpecial = false;
		if (enemy->GetIsPhase2() && static_cast<float>(enemy->GetHP()) <= static_cast<float>(enemy->GetMaxHP()) * 0.5f) {
			useSpecial = (static_cast<int>(rand() % 100) < 70);
		}

		if (useSpecial) {
			currentAttackType_ = AttackType::kRangedSpecial;
			rangedAttackSpecial_->Start(enemy, player);
		}
		else {
			currentAttackType_ = AttackType::kRanged;
			rangedAttack_->Start(enemy, player);
		}
	}

	attackPreparationTimer_ = 0;
}

// =============================================================
//  ResetAttack
// =============================================================
void EnemyAttackManager::ResetAttack()
{
	currentAttackType_ = AttackType::kNone;
	attackPreparationTimer_ = 0;
}

// =============================================================
//  DebugTriggerAttack
// =============================================================
void EnemyAttackManager::DebugTriggerAttack(EnemyAttackManager::AttackType type, Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;
	ResetAttack();
	currentAttackType_ = type;

	switch (type) {
	case AttackType::kMelee:
		meleeAttack_->Start(enemy, player);
		break;
	case AttackType::kRanged:
		rangedAttack_->Start(enemy, player);
		break;
	case AttackType::kRangedSpecial:
		rangedAttackSpecial_->Start(enemy, player);
		break;
	default:
		currentAttackType_ = AttackType::kNone;
		break;
	}
}

// =============================================================
//  InterruptByRush
// =============================================================
void EnemyAttackManager::InterruptByRush(Enemy* enemy)
{
	if (currentAttackType_ == AttackType::kMelee) {
		meleeAttack_->Interrupt(enemy);
	}
	else if (currentAttackType_ == AttackType::kRanged) {
		rangedAttack_->Interrupt();
	}
	else if (currentAttackType_ == AttackType::kRangedSpecial) {
		rangedAttackSpecial_->Interrupt();
	}

	ResetAttack();
}