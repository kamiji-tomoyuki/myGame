#pragma once
#include <d3d12.h>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "array"
#include "wrl.h"

#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include <Matrix4x4.h>
#include <Quaternion.h>

// --- モデル関連構造体 ---

/// <summary>
/// クォータニオン変換
/// </summary>
struct QuaternionTransform {
	Vector3 scale;
	Quaternion rotate;
	Vector3 translate;
};

/// <summary>
/// 頂点データ
/// </summary>
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

/// <summary>
/// マテリアルデータ
/// </summary>
struct MaterialData
{
	std::string textureFilePath;
	uint32_t textureIndex = 0;
};

/// <summary>
/// ノード
/// </summary>
struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node>children;
};

/// <summary>
/// ジョイント
/// </summary>
struct Joint {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	Matrix4x4 skeletonSpaceMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t>parent;
};

/// <summary>
/// スケルトン
/// </summary>
struct Skeleton {
	int32_t root;
	std::map<std::string, int32_t>jointMap;
	std::vector<Joint>joints;
};

/// <summary>
/// 頂点ウェイトデータ
/// </summary>
struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

/// <summary>
/// ジョイントウェイトデータ
/// </summary>
struct JointWeightData {
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData>vertexWeights;
};

/// <summary>
/// メッシュデータ
/// </summary>
struct MeshData {
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialData material;
	std::map<std::string, JointWeightData> skinClusterData;
};

/// <summary>
/// モデルデータ
/// </summary>
struct ModelData {
	std::vector<MeshData> meshes;
	Node rootNode;
};

/// <summary>
/// メッシュリソース
/// </summary>
struct MeshResources {
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	VertexData* vertexData;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	uint32_t* indexData;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	uint32_t materialIndex;
	size_t indexCount;
};

/// <summary>
/// 頂点インフルエンス
/// </summary>
static const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices;
};

/// <summary>
/// GPU用スケルトン
/// </summary>
struct WellForGPU {
	Matrix4x4 skeletonSpaceMatrix;
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

/// <summary>
/// スキンクラスタ
/// </summary>
struct SkinCluster {
	std::vector<Matrix4x4>inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource>influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence>mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>paletteSrvHandle;
};

/// <summary>
/// キーフレーム(Vector3)
/// </summary>
struct KeyframeVector3 {
	Vector3 value;
	float time;
};

/// <summary>
/// キーフレーム(Quaternion)
/// </summary>
struct KeyframeQuaternion {
	Quaternion value;
	float time;
};

/// <summary>
/// ノードアニメーション
/// </summary>
struct NodeAnimation {
	std::vector<KeyframeVector3> translate;
	std::vector<KeyframeQuaternion> rotate;
	std::vector<KeyframeVector3> scale;
};

/// <summary>
/// アニメーション
/// </summary>
struct Animation {
	float duration;
	std::map<std::string, NodeAnimation>nodeAnimations;
};