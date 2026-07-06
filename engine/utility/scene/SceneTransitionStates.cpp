#include "SceneTransitionStates.h"
#include "SceneTransition.h"

// --- SceneTransitionStateNone ---
namespace Engine {
void SceneTransitionStateNone::Update(SceneTransition* context) {
    // 何もしない
}

void SceneTransitionStateNone::Draw(SceneTransition* context) {
    // 何もしない
}

// --- SceneTransitionStateFadeIn ---
void SceneTransitionStateFadeIn::Update(SceneTransition* context) {
    context->ProcFadeIn();
    context->UpdateGrid();
    
    if (context->FadeInFinish()) {
        context->ChangeState(std::make_unique<SceneTransitionStateWait>());
    }
}

void SceneTransitionStateFadeIn::Draw(SceneTransition* context) {
    context->DrawGrid();
}

// --- SceneTransitionStateWait ---
void SceneTransitionStateWait::Update(SceneTransition* context) {
    // フェードアウト開始待ち
    context->UpdateGrid();
}

void SceneTransitionStateWait::Draw(SceneTransition* context) {
    context->DrawGrid();
}

// --- SceneTransitionStateFadeOut ---
void SceneTransitionStateFadeOut::Update(SceneTransition* context) {
    context->ProcFadeOut();
    context->UpdateGrid();
    
    if (context->IsEnd()) {
        context->ChangeState(std::make_unique<SceneTransitionStateNone>());
    }
}

void SceneTransitionStateFadeOut::Draw(SceneTransition* context) {
    context->DrawGrid();
}
} // namespace Engine
