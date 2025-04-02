#include "Skin.h"
#include <cassert>

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
	srvManager_->CreateSRVforStructuredBuffer(skinClusterSrvIndex_, skinCluster.paletteResource.Get(), UINT(skeleton.joints.size()), sizeof(WellForGPU));


	// --- mappedInfluence作成 ---
	skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.mappedInfluence = { mappedInfluence,modelData.vertices.size() };

	// --- BufferView作成 ---
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// --- inverseBindPoseMatrices作成 ---
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), []() { return MakeIdentity4x4(); });

	// --- SkinClusterを解析、Influenceの中身を作成 ---
	for (const auto& jointWeight : modelData.skinClusterData) {
		auto it = skeleton.jointMap.find(jointWeight.first);
		if (it == skeleton.jointMap.end()) {
			continue;
		}
		
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster;
}
