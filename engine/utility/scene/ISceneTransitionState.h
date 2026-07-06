#pragma once

namespace Engine {
class SceneTransition;

/// <summary>
/// シーン遷移状態の基底クラス
/// </summary>
class ISceneTransitionState {
  public:
    virtual ~ISceneTransitionState() = default;
    virtual void Update(SceneTransition *context) = 0;
    virtual void Draw(SceneTransition *context) = 0;
};
} // namespace Engine
