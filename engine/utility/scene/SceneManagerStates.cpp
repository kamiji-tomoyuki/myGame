#include "SceneManagerStates.h"
#include "SceneManager.h"
#include "BaseScene.h"

void SceneManagerNormalState::Update(SceneManager* manager) {
    // 実行中シーンを更新する
    if (manager->GetBaseScene()) {
        manager->GetBaseScene()->Update();
    }
}

void SceneManagerTransitionState::Update(SceneManager* manager) {
    // トランジションの更新
    if (!manager->GetTransitionEnd()) {
        manager->UpdateTransition();
    } else {
        // トランジション終了したら通常状態に戻る
        manager->ChangeState(std::make_unique<SceneManagerNormalState>());
    }

    // 次のシーンの予約があるなら
    if (manager->HasNextScene()) {
        manager->SceneChange();
    }

    // 実行中シーンを更新する（遷移中も裏で動かす場合はここで呼ぶ）
    if (manager->GetBaseScene()) {
        manager->GetBaseScene()->Update();
    }
}
