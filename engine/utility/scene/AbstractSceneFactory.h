#pragma once
#include"BaseScene.h"
#include"string"
#include<memory>

/// <summary>
/// シーン生成用基底クラス
/// </summary>
namespace Engine {
class AbstractSceneFactory 
{
public:
	
	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~AbstractSceneFactory() = default;

	/// <summary>
	/// シーン生成
	/// </summary>
	/// <param name="sceneName"></param>
	/// <returns></returns>
	virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};
} // namespace Engine
