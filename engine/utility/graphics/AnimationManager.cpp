#include "AnimationManager.h"
AnimationManager* AnimationManager::instance = nullptr;

AnimationManager* AnimationManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new AnimationManager;
	}
	return instance;
}

void AnimationManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void AnimationManager::Initialize()
{

}

ModelAnimation* AnimationManager::FindAnimation(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if (animations.contains(filePath)) {
		// 読み込みモデルを戻り値としてreturn
		return animations.at(filePath).get();
	}

	// ファイル名一致なし
	return nullptr;
}

void AnimationManager::LoadAnimation(const std::string& filePath,ModelData modelData)
{
	// 読み込み済みモデルを探索
	if (animations.contains(filePath)) {
		// 読み込み済みなら早期リターン
		return;
	}

	// モデルの生成とファイル読み込み、初期化
	std::unique_ptr<ModelAnimation> animator = std::make_unique<ModelAnimation>();
	animator->SetModelData(modelData);
	animator->Initialize("resources/models/", filePath);

	// モデルをmapコンテナに格納する
	animations.insert(std::make_pair(filePath, std::move(animator)));
}
