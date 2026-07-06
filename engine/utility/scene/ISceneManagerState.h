#pragma once

namespace Engine {
class SceneManager;

/// <summary>
/// シーンマネージャーの状態基底クラス
/// </summary>
class ISceneManagerState {
public:
    virtual ~ISceneManagerState() = default;
    virtual void Update(SceneManager* manager) = 0;
};
} // namespace Engine
