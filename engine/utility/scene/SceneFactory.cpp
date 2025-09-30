#include "SceneFactory.h"
#include"TitleScene.h"
#include"GameScene.h"
#include"GameClearScene.h"
#include"GameOverScene.h"

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
	return newScene;
}
