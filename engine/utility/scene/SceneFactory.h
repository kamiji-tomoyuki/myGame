#pragma once
#include"AbstractSceneFactory.h"

/// <summary>
/// シーン生成クラス
/// </summary>
namespace Engine {
class SceneFactory :public AbstractSceneFactory
{
public:

	/// <summary>
    /// シーン生成
    /// </summary>
    /// <param name="sceneName"></param>
    /// <returns></returns>
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;

};

} // namespace Engine
