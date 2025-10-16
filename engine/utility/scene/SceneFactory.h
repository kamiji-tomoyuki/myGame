#pragma once
#include"AbstractSceneFactory.h"

/// <summary>
/// シーン生成クラス
/// </summary>
class SceneFactory :public AbstractSceneFactory
{
public:

	/// <summary>
    /// シーン生成
    /// </summary>
    /// <param name="sceneName"></param>
    /// <returns></returns>
    BaseScene* CreateScene(const std::string& sceneName) override;

};

