#include "Skin.h"
#include <cassert>
#include <algorithm>

#include "SrvManager.h"
#include <DirectXCommon.h>

#include <myMath.h>

void Skin::Initialize(const Skeleton& skeleton, const ModelData& modelData)
{
	// --- スキンクラスター生成 ---
	skinCluster_ = CreateSkinCluster(skeleton,modelData);
}

void Skin::Update(const Skeleton& skeleton)
{
	// --- 更新 ---
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster_.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

SkinCluster Skin::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData)
{
	SkinCluster skinCluster;
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	SrvManager* srvManager_ = SrvManager::GetInstance();

	// --- paletteSrvHandle作成 ---
	skinCluster.paletteResource = dxCommon->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() };
	skinClusterSrvIndex_ = srvManager_->Allocate() + 1;
	skinCluster.paletteSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterSrvIndex_);
	skinCluster.paletteSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterSrvIndex_);

	// --- palette用SRV作成 ---
	srvManager_->CreateSRVforStructuredBuffer(
		skinClusterSrvIndex_,
		skinCluster.paletteResource.Get(),
		UINT(skeleton.joints.size()),
		sizeof(WellForGPU)
	);

	// --- モデル全体の頂点数を計算 ---
	size_t totalVertexCount = 0;
	for (const auto& mesh : modelData.meshes) {
		totalVertexCount += mesh.vertices.size();
	}

	// --- mappedInfluence作成 ---
	skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * totalVertexCount);
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * totalVertexCount);
	skinCluster.mappedInfluence = { mappedInfluence,totalVertexCount };

	// --- BufferView作成 ---
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * totalVertexCount);
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// --- inverseBindPoseMatrices作成 ---
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(
		skinCluster.inverseBindPoseMatrices.begin(),
		skinCluster.inverseBindPoseMatrices.end(),
		[]() { return MakeIdentity4x4(); }
	);

	// --- 各メッシュの SkinClusterData を解析 ---
	size_t vertexOffset = 0;
	for (const auto& mesh : modelData.meshes) {
		for (const auto& jointWeight : mesh.skinClusterData) {
			auto it = skeleton.jointMap.find(jointWeight.first);
			if (it == skeleton.jointMap.end()) {
				continue;
			}

			// inverseBindPoseMatrix 登録
			skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

			// 頂点の影響度登録
			for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
				size_t globalVertexIndex = vertexOffset + vertexWeight.vertexIndex;
				auto& currentInfluence = skinCluster.mappedInfluence[globalVertexIndex];
				for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
					if (currentInfluence.weights[index] == 0.0f) {
						currentInfluence.weights[index] = vertexWeight.weight;
						currentInfluence.jointIndices[index] = (*it).second;
						break;
					}
				}
			}
		}
		vertexOffset += mesh.vertices.size();
	}

	return skinCluster;
}