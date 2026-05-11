#include "EnemyStatePlaying.h"
#include "EnemyStateGameOver.h"
#include "EnemyStateGameClear.h"
#include "Enemy.h"
#include "EnemyMove.h"
#include "EnemyHitReaction.h"
#include "EnemyAttackManager.h"
#include <memory>

void EnemyStatePlaying::Enter(Enemy* enemy)
{
	// 特に初期化なし
}

std::unique_ptr<IEnemyState> EnemyStatePlaying::Update(Enemy* enemy)
{
	// ゲームオーバー遷移チェック
	if (enemy->GetGameState() == Enemy::GameState::kGameOver) {
		return std::make_unique<EnemyStateGameOver>();
	}

	// ゲームクリア遷移チェック
	if (enemy->GetGameState() == Enemy::GameState::kGameClear) {
		return std::make_unique<EnemyStateGameClear>();
	}

	EnemyHitReaction* hitReaction = enemy->GetHitReaction();
	EnemyMove* move = enemy->GetMove();
	EnemyAttackManager* attackManager = enemy->GetAttackManager();

	// スタン中は行動をスキップ
	if (hitReaction->IsStunned()) {
		hitReaction->UpdateStun();
		return nullptr;
	}

	// ラッシュ状態チェック
	hitReaction->CheckPlayerRushStatus(enemy->GetPlayer());

	// ノックバック中は専用更新、通常時は移動＋攻撃
	if (hitReaction->IsBeingKnockedBack()) {
		hitReaction->UpdateKnockback(enemy);
	}
	else {
		move->Update(enemy, enemy->GetPlayer());
		if (attackManager) {
			attackManager->Update(enemy, enemy->GetPlayer());
		}
	}

	return nullptr;
}

void EnemyStatePlaying::Exit(Enemy* enemy)
{
	// 攻撃を中断する
	if (EnemyAttackManager* mgr = enemy->GetAttackManager()) {
		mgr->InterruptByRush(enemy);
	}
}