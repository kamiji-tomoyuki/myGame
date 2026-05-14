#include "SceneFactory.h"
#include"TitleScene.h"
#include"GameScene.h"
#include"GameClearScene.h"
#include"GameOverScene.h"
#include"debugScene/ParticleDebugScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
   // 次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
	}
	else if (sceneName == "GAME") {
		newScene = new GameScene();
	}
	else if (sceneName == "CLEAR") {
		newScene = new GameClearScene();
	}
	else if (sceneName == "OVER") {
		newScene = new GameOverScene();
	}
	else if (sceneName == "PARTICLE_DEBUG") {
		newScene = new ParticleDebugScene();
	}
	return newScene;
}
