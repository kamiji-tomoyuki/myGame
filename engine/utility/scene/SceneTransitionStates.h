#pragma once
#include "ISceneTransitionState.h"

/// <summary>
/// 未遷移状態
/// </summary>
namespace Engine {
class SceneTransitionStateNone : public ISceneTransitionState {
public:
    void Update(SceneTransition* context) override;
    void Draw(SceneTransition* context) override;
};

/// <summary>
/// フェードイン状態（画面が黒く埋まる）
/// </summary>
class SceneTransitionStateFadeIn : public ISceneTransitionState {
public:
    void Update(SceneTransition* context) override;
    void Draw(SceneTransition* context) override;
};

/// <summary>
/// フェードイン完了・待ち状態
/// </summary>
class SceneTransitionStateWait : public ISceneTransitionState {
public:
    void Update(SceneTransition* context) override;
    void Draw(SceneTransition* context) override;
};

/// <summary>
/// フェードアウト状態（次シーンへ移行）
/// </summary>
class SceneTransitionStateFadeOut : public ISceneTransitionState {
public:
    void Update(SceneTransition* context) override;
    void Draw(SceneTransition* context) override;
};
} // namespace Engine
