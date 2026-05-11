#include "EnemyAttackManager.h"
#include "Enemy.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"
#include "Player.h"

// =============================================================
//  関数ポインタテーブルの定義
// =============================================================
const std::unordered_map<EnemyAttackManager::AttackType, EnemyAttackManager::UpdateFunc>
EnemyAttackManager::kUpdateTable_ = {
	{ AttackType::kMelee,  &EnemyAttackManager::UpdateMelee  },
	{ AttackType::kRanged, &EnemyAttackManager::UpdateRanged },
};

const std::unordered_map<EnemyAttackManager::AttackType, EnemyAttackManager::CompleteFunc>
EnemyAttackManager::kCompleteTable_ = {
	{ AttackType::kMelee,  &EnemyAttackManager::IsCompleteMelee  },
	{ AttackType::kRanged, &EnemyAttackManager::IsCompleteRanged },
};

EnemyAttackManager::EnemyAttackManager()
{
	meleeAttack_ = std::make_unique<EnemyAttackMelee>();
	rangedAttack_ = std::make_unique<EnemyAttackRanged>();
}

EnemyAttackManager::~EnemyAttackManager() = default;

void EnemyAttackManager::Initialize()
{
	meleeAttack_->Initialize();
	rangedAttack_->Initialize();
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
		if (attackPreparationTimer_ >= kAttackPreparationTime_) {
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

	float distanceToPlayer =
		(player->GetCenterPosition() - enemy->GetCenterPosition()).Length();

	if (distanceToPlayer <= kMeleeAttackRange_) {
		currentAttackType_ = AttackType::kMelee;
		meleeAttack_->Start(enemy, player);
	}
	else {
		currentAttackType_ = AttackType::kRanged;
		rangedAttack_->Start(enemy, player);
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

	ResetAttack();
}