#pragma once
#include "ISceneManagerState.h"

/// <summary>
/// 通常更新状態
/// </summary>
namespace Engine {
class SceneManagerNormalState : public ISceneManagerState {
public:
    void Update(SceneManager* manager) override;
};

/// <summary>
/// シーン遷移中状態
/// </summary>
class SceneManagerTransitionState : public ISceneManagerState {
public:
    void Update(SceneManager* manager) override;
};
} // namespace Engine
