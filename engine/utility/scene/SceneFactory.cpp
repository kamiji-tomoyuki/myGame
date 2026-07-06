#include "SceneFactory.h"
#include"TitleScene.h"
#include"GameScene.h"
#include"GameClearScene.h"
#include"GameOverScene.h"
#include"debugScene/ParticleDebugScene.h"

namespace Engine {
std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
   // 次のシーンを生成
	std::unique_ptr<BaseScene> newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = std::make_unique<TitleScene>();
	}
	else if (sceneName == "GAME") {
		newScene = std::make_unique<GameScene>();
	}
	else if (sceneName == "CLEAR") {
		newScene = std::make_unique<GameClearScene>();
	}
	else if (sceneName == "OVER") {
		newScene = std::make_unique<GameOverScene>();
	}
	else if (sceneName == "PARTICLE_DEBUG") {
		newScene = std::make_unique<ParticleDebugScene>();
	}
	return newScene;
}
} // namespace Engine
