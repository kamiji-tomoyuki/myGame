#include "EnemyStatePlaying.h"
#include "EnemyStateGameOver.h"
#include "EnemyStateGameClear.h"
#include "EnemyStateTransformation.h"
#include "Enemy.h"
#include "EnemyMove.h"
#include "EnemyHitReaction.h"
#include "EnemyAttackManager.h"
#include <memory>

using namespace Engine;
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

	// 強化演出遷移チェック
	if (enemy->GetHP() <= enemy->GetMaxHP() / 2 && !enemy->GetIsPhase2()) {
		return std::make_unique<EnemyStateTransformation>();
	}

	EnemyHitReaction* hitReaction = enemy->GetHitReaction();
	EnemyMove* move = enemy->GetMove();
	EnemyAttackManager* attackManager = enemy->GetAttackManager();

	// ラッシュ状態チェック
	hitReaction->CheckPlayerRushStatus(enemy);

	// 連続ヒット対策：一定連続で被弾したら攻撃を中断してプレイヤーを押し返す
	hitReaction->UpdateComboRepel(enemy);

	// ノックバック中は専用更新（スタン中より優先して即座に開始させる）
	if (hitReaction->IsBeingKnockedBack()) {
		hitReaction->UpdateKnockback(enemy);
		// ノックバック中もスタン時間は進める
		if (hitReaction->IsStunned()) {
			hitReaction->UpdateStun();
		}
		return nullptr;
	}

	// スタン中は行動をスキップ
	if (hitReaction->IsStunned()) {
		hitReaction->UpdateStun();
		return nullptr;
	}

	// 通常時は移動＋攻撃
	move->Update(enemy, enemy->GetPlayer());
	if (attackManager) {
		attackManager->Update(enemy, enemy->GetPlayer());
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