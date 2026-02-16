#include "EnemyAttackManager.h"
#include "Enemy.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"
#include "Player.h"

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

void EnemyAttackManager::Update(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;

	// 攻撃中でない場合は準備タイマーを更新
	if (currentAttackType_ == AttackType::kNone) {
		attackPreparationTimer_++;

		// 準備時間が完了したら攻撃を選択
		if (attackPreparationTimer_ >= kAttackPreparationTime_) {
			SelectAndStartAttack(enemy, player);
		}
		return;
	}

	// 各攻撃の更新処理
	switch (currentAttackType_) {
	case AttackType::kMelee:
		meleeAttack_->Update(enemy, player);

		// 攻撃完了チェック
		if (meleeAttack_->IsComplete()) {
			ResetAttack();
		}
		break;

	case AttackType::kRanged:
		rangedAttack_->Update(enemy, player);

		// 攻撃完了チェック
		if (rangedAttack_->IsComplete()) {
			ResetAttack();
		}
		break;
	}
}

void EnemyAttackManager::SelectAndStartAttack(Enemy* enemy, Player* player)
{
	if (enemy == nullptr || player == nullptr) return;

	// プレイヤーとの距離を計算
	float distanceToPlayer = (player->GetCenterPosition() - enemy->GetCenterPosition()).Length();

	// 距離に応じて攻撃タイプを選択
	if (distanceToPlayer <= kMeleeAttackRange_) {
		// 近距離なら突進攻撃
		currentAttackType_ = AttackType::kMelee;
		meleeAttack_->Start(enemy, player);
	}
	else {
		// 遠距離なら遠距離攻撃
		currentAttackType_ = AttackType::kRanged;
		rangedAttack_->Start(enemy, player);
	}

	attackPreparationTimer_ = 0;
}

bool EnemyAttackManager::IsAttackComplete() const
{
	switch (currentAttackType_) {
	case AttackType::kMelee:
		return meleeAttack_->IsComplete();
	case AttackType::kRanged:
		return rangedAttack_->IsComplete();
	case AttackType::kNone:
		return true;
	}
	return true;
}

void EnemyAttackManager::ResetAttack()
{
	currentAttackType_ = AttackType::kNone;
	attackPreparationTimer_ = 0;
}

void EnemyAttackManager::InterruptByRush()
{
	// 現在の攻撃を中断
	if (currentAttackType_ == AttackType::kMelee) {
		meleeAttack_->Interrupt();
	}
	else if (currentAttackType_ == AttackType::kRanged) {
		rangedAttack_->Interrupt();
	}

	ResetAttack();
}