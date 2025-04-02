#pragma once
#include"map"
#include"string"
#include"memory"
#include"animation/ModelAnimation.h"
#include <SrvManager.h>
class AnimationManager
{
private:
	static AnimationManager* instance;

	AnimationManager() = default;
	~AnimationManager() = default;
	AnimationManager(AnimationManager&) = default;
	AnimationManager& operator=(AnimationManager&) = default;
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon"></param>
	void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static AnimationManager* GetInstance();

	/// <summary>
	/// モデルの検索
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	ModelAnimation* FindAnimation(const std::string& filePath);

	/// <summary>
	/// モデルファイルの読み込み
	/// </summary>
	/// <param name="filePath"></param>
	void LoadAnimation(const std::string& filePath, ModelData modelData);
public:
	// モデルデータ
	std::map<std::string, std::unique_ptr<ModelAnimation>> animations;
};

