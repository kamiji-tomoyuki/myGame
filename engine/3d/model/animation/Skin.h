#pragma once
#include <ModelStructs.h>

#include <cstdint>

// スキニング
class Skin
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const Skeleton& skeleton, const ModelData& modelData);
	
	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(const Skeleton& skeleton);

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	uint32_t GetSrvIndex() { return skinClusterSrvIndex_; }
	SkinCluster GetSkinCluster() { return skinCluster_; }

private:

	/// <summary>
	/// SkinClusterの生成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="skeleton"></param>
	/// <param name="modelData"></param>
	/// <param name="descriptorHeap"></param>
	/// <param name="descriptorSize"></param>
	/// <returns></returns>
	SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);

private:

	SkinCluster skinCluster_;
	uint32_t skinClusterSrvIndex_ = 0;
};

